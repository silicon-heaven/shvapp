#include "application.h"
#include "appclioptions.h"
#include "checklogtask.h"
#include "devicemonitor.h"
#include "logsanitizer.h"

#include <shv/coreqt/log.h>

#include <shv/core/utils/shvjournalentry.h>

using namespace shv::core::utils;

LogSanitizer::LogSanitizer(QObject *parent)
	: QObject(parent)
	, m_lastCheckedDevice(-1)
	, m_timer(this)
{
	connect(&m_timer, &QTimer::timeout, this, qOverload<>(&LogSanitizer::sanitizeLogCache));
	Application *app = Application::instance();
	connect(app->deviceConnection(), &shv::iotqt::rpc::DeviceConnection::stateChanged, this, &LogSanitizer::onShvStateChanged);
	connect(app, &Application::deviceDataChanged, this, &LogSanitizer::onDeviceDataChanged);

	DeviceMonitor *monitor = app->deviceMonitor();
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

void LogSanitizer::trimDirtyLog(const QString &site_path)
{
	if (Application::instance()->deviceMonitor()->isPushLogDevice(site_path)) {
		SHV_EXCEPTION("Can not trim dirty log of pushLog device");
	}

	if (Application::instance()->deviceMonitor()->onlineDevices().contains(site_path)) {
		CheckLogTask *running_task = findChild<CheckLogTask*>(site_path, Qt::FindChildOption::FindDirectChildrenOnly);
		if (running_task) {
			if (running_task->checkType() == CheckType::TrimDirtyLogOnly || running_task->checkType() == CheckType::ReplaceDirtyLog) {
				return;
			}
			SHV_EXCEPTION("Device " + site_path.toStdString() + " is currently sanitized.");
		}
		else {
			shvMessage() << "trim dirty log for" << site_path;
			CheckLogTask *task = new CheckLogTask(site_path, CheckType::TrimDirtyLogOnly, this);
			task->setObjectName(site_path);
			connect(task, &CheckLogTask::finished, [site_path, task](bool success) {
				task->setParent(nullptr);
				task->deleteLater();
				shvMessage() << "trimming dirty log for" << site_path << (success ? "succesfully finished" : "finished with error");
			});
			task->exec();
		}
	}
	else {
		SHV_EXCEPTION("Device " + site_path.toStdString() + " is offline.");
	}
}

void LogSanitizer::onDeviceAppeared(const QString &site_path)
{
	if (Application::instance()->deviceMonitor()->isPushLogDevice(site_path)) {
		return;
	}
	planDirtyLogTrim(site_path);
	setupTimer();
}

void LogSanitizer::checkNewDevicesQueue()
{
	if (!m_newDevices.isEmpty() && findChildren<CheckLogTask*>(QString(), Qt::FindChildOption::FindDirectChildrenOnly).count() == 0) {
		QString site_path = m_newDevices.takeFirst();
		sanitizeLogCache(site_path, CheckType::ReplaceDirtyLog);
	}

}

void LogSanitizer::planDirtyLogTrim(const QString &site_path)
{
	QTimer *timer = m_newDeviceTimers.value(site_path);
	if (!timer) {
		timer = new QTimer(this);
		timer->setInterval(30000);  //let device to collect snapshot
		timer->setSingleShot(true);
		connect(timer, &QTimer::timeout, this, [this, site_path]() {
			m_newDeviceTimers[site_path]->deleteLater();
			m_newDeviceTimers.remove(site_path);
			m_newDevices << site_path;
			checkNewDevicesQueue();
		});
		m_newDeviceTimers[site_path] = timer;
	}
	timer->start();
}

void LogSanitizer::onDeviceDataChanged(const QString &site_path, const QString &property, const QString &method, const shv::chainpack::RpcValue &data)
{
	Q_UNUSED(method);

	if (!Application::instance()->deviceMonitor()->isPushLogDevice(site_path)) {
		if (property.startsWith(".app/connector/") && property.endsWith("/state")) {
			shv::chainpack::RpcValue value = data;
			if (ShvJournalEntry::isShvJournalEntry(value)) {
				value = ShvJournalEntry::fromRpcValue(value).value;
			}
			else if (shv::chainpack::DataChange::isDataChange(value)) {
				value = shv::chainpack::DataChange::fromRpcValue(value).value();
			}

			if (value.asString() == "Connected") {
				planDirtyLogTrim(site_path);
			}
		}
	}
}

void LogSanitizer::onDeviceDisappeared(const QString &site_path)
{
	setupTimer();
	if (m_newDeviceTimers.contains(site_path)) {
		m_newDeviceTimers[site_path]->stop();
		m_newDeviceTimers[site_path]->deleteLater();
		m_newDeviceTimers.remove(site_path);
	}
	m_newDevices.removeOne(site_path);
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

bool LogSanitizer::sanitizeLogCache(const QString &site_path, CheckType check_type)
{
	if (Application::instance()->deviceConnection()->state() != shv::iotqt::rpc::ClientConnection::State::BrokerConnected) {
		return false;
	}
	if (findChild<CheckLogTask*>(site_path, Qt::FindChildOption::FindDirectChildrenOnly)) {
		logSanitizing() << "seems that sanitizing for" << site_path << "is already running, giving up";
		return false;
	}
	if (m_newDeviceTimers.keys().contains(site_path)) {
		return false;
	}
	if (m_newDevices.contains(site_path)) {
		return false;
	}

	logSanitizing() << "checking logs for" << site_path;
	CheckLogTask *task = new CheckLogTask(site_path, check_type, this);
	task->setObjectName(site_path);
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
