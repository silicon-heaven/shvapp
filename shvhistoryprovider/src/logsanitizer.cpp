#include "application.h"
#include "appclioptions.h"
#include "checklogtask.h"
#include "devicemonitor.h"
#include "logsanitizer.h"

#include <shv/coreqt/log.h>

LogSanitizer::LogSanitizer(QObject *parent)
	: QObject(parent)
	, m_lastCheckedDevice(-1)
	, m_timer(this)
{
	connect(&m_timer, &QTimer::timeout, this, qOverload<>(&LogSanitizer::sanitizeLogCache));
	connect(Application::instance()->deviceConnection(), &shv::iotqt::rpc::DeviceConnection::stateChanged, this, &LogSanitizer::onShvStateChanged);

	DeviceMonitor *monitor = Application::instance()->deviceMonitor();
	connect(monitor, &DeviceMonitor::deviceConnectedToBroker, this, &LogSanitizer::onDeviceAppeared);
	connect(monitor, &DeviceMonitor::deviceDisconnectedFromBroker, this, &LogSanitizer::onDeviceDisappeared);

	setupTimer();
}

void LogSanitizer::setupTimer()
{
	int device_count = Application::instance()->deviceMonitor()->onlineDevices().count();
	if (device_count == 0) {
		device_count = 1;
	}
	int interval = Application::instance()->cliOptions()->trimDirtyLogInterval() * 1000 * 60 / device_count;
	if (interval != m_interval) {
		m_interval = interval;
		int remaining = interval - (m_timer.interval() - m_timer.remainingTime());
		if (remaining > 0) {
			m_timer.start(remaining);
		}
		else {
			m_timer.start(m_interval);
			sanitizeLogCache();
		}
		logSanitizing() << "setup timer" << "device count" << device_count << "interval" << interval;
	}
}

void LogSanitizer::trimDirtyLog(const QString &shv_path)
{
	if (Application::instance()->deviceMonitor()->isPushLogDevice(shv_path)) {
		SHV_EXCEPTION("Can not trim dirty log of pushLog device");
	}

	if (Application::instance()->deviceMonitor()->onlineDevices().contains(shv_path)) {
		CheckLogTask *running_task = findChild<CheckLogTask*>(shv_path, Qt::FindChildOption::FindDirectChildrenOnly);
		if (running_task) {
			if (running_task->checkType() == CheckType::TrimDirtyLogOnly || running_task->checkType() == CheckType::ReplaceDirtyLog) {
				return;
			}
			SHV_EXCEPTION("Device " + shv_path.toStdString() + " is currently sanitized.");
		}
		else {
			shvMessage() << "trim dirty log for" << shv_path;
			CheckLogTask *task = new CheckLogTask(shv_path, CheckType::TrimDirtyLogOnly, this);
			task->setObjectName(shv_path);
			connect(task, &CheckLogTask::finished, [shv_path, task](bool success) {
				task->setParent(nullptr);
				task->deleteLater();
				shvMessage() << "trimming dirty log for" << shv_path << (success ? "succesfully finished" : "finished with error");
			});
			task->exec();
		}
	}
	else {
		SHV_EXCEPTION("Device " + shv_path.toStdString() + " is offline.");
	}
}

void LogSanitizer::onDeviceAppeared(const QString &shv_path)
{
	if (Application::instance()->deviceMonitor()->isPushLogDevice(shv_path)) {
		return;
	}
	QTimer *timer = m_newDeviceTimers.value(shv_path);
	if (!timer) {
		timer = new QTimer(this);
		timer->setInterval(60000);  //let device to collect snapshot
		timer->setSingleShot(true);
		connect(timer, &QTimer::timeout, this, [this, shv_path]() {
			m_newDeviceTimers[shv_path]->deleteLater();
			m_newDeviceTimers.remove(shv_path);
			m_newDevices << shv_path;
			checkNewDevicesQueue();
		});
		m_newDeviceTimers[shv_path] = timer;
		timer->start();
	}
	setupTimer();
}

void LogSanitizer::checkNewDevicesQueue()
{
	if (!m_newDevices.isEmpty() && findChildren<CheckLogTask*>(QString(), Qt::FindChildOption::FindDirectChildrenOnly).count() == 0) {
		QString shv_path = m_newDevices.takeFirst();
		sanitizeLogCache(shv_path, CheckType::ReplaceDirtyLog);
	}

}

void LogSanitizer::onDeviceDisappeared(const QString &shv_path)
{
	setupTimer();
	if (m_newDeviceTimers.contains(shv_path)) {
		m_newDeviceTimers[shv_path]->stop();
		m_newDeviceTimers[shv_path]->deleteLater();
		m_newDeviceTimers.remove(shv_path);
	}
	m_newDevices.removeOne(shv_path);
}

void LogSanitizer::sanitizeLogCache()
{
	logSanitizing() << "preparing to sanitize" << findChildren<CheckLogTask*>(QString(), Qt::FindChildOption::FindDirectChildrenOnly).count();
	if (Application::instance()->deviceConnection()->state() != shv::iotqt::rpc::ClientConnection::State::BrokerConnected) {
		return;
	}
	if (m_interval != m_timer.interval()) {
		m_timer.setInterval(m_interval);
	}
	if (findChildren<CheckLogTask*>(QString(), Qt::FindChildOption::FindDirectChildrenOnly).count() > 0) {
		logSanitizing() << "giving up, other checklog task is running";
		return;
	}
	QStringList online_devices = Application::instance()->deviceMonitor()->onlineDevices();
	QStringList all_devices = Application::instance()->deviceMonitor()->monitoredDevices();
	QStringList sanitized_devices;
	for (int i = 0; i < all_devices.count(); ++i) {
		QString device = all_devices[i];
		bool is_push_log = Application::instance()->deviceMonitor()->isPushLogDevice(device);
		if ((online_devices.contains(device) && !is_push_log) ||
			(is_push_log && !Application::instance()->deviceMonitor()->syncLogBroker(device).isEmpty())) {
			sanitized_devices << device;
		}
	}
	if (sanitized_devices.count() == 0) {
		return;
	}
	if (++m_lastCheckedDevice >= sanitized_devices.count()) {
		m_lastCheckedDevice = 0;
	}
	logSanitizing() << "sanitizing" << sanitized_devices[m_lastCheckedDevice];
	sanitizeLogCache(sanitized_devices[m_lastCheckedDevice], CheckType::CheckDirtyLogState);
}

bool LogSanitizer::sanitizeLogCache(const QString &shv_path, CheckType check_type)
{
	if (Application::instance()->deviceConnection()->state() != shv::iotqt::rpc::ClientConnection::State::BrokerConnected) {
		return false;
	}
	if (findChild<CheckLogTask*>(shv_path, Qt::FindChildOption::FindDirectChildrenOnly)) {
		logSanitizing() << "seems that sanitizing for" << shv_path << "is already running, giving up";
		return false;
	}
	if (m_newDeviceTimers.keys().contains(shv_path)) {
		return false;
	}
	if (m_newDevices.contains(shv_path)) {
		return false;
	}

	logSanitizing() << "checking logs for" << shv_path;
	CheckLogTask *task = new CheckLogTask(shv_path, check_type, this);
	task->setObjectName(shv_path);
	connect(task, &CheckLogTask::finished, [this, task](bool success) {
		task->setParent(nullptr);
		task->deleteLater();
		logSanitizing() << "checking logs for" << task->objectName() << (success ? "succesfully finished" : "finished with error");
		checkNewDevicesQueue();
	});
	task->exec();
	return true;
}

void LogSanitizer::onShvStateChanged()
{
	if (Application::instance()->deviceConnection()->state() == shv::iotqt::rpc::ClientConnection::State::NotConnected) {
		m_timer.stop();
		for (QTimer *timer : m_newDeviceTimers) {
			timer->stop();
			timer->deleteLater();
		}
		m_newDeviceTimers.clear();
		m_newDevices.clear();
	}
}
