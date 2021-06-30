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
	m_timer.start(interval);
}

void LogSanitizer::trimDirtyLog(const QString &shv_path)
{
	if (Application::instance()->deviceMonitor()->isPushLogDevice(shv_path)) {
		SHV_EXCEPTION("Can not trim dirty log of pushLog device");
	}

	if (Application::instance()->deviceMonitor()->onlineDevices().contains(shv_path)) {
		CheckLogTask *running_task = findChild<CheckLogTask*>(shv_path, Qt::FindChildOption::FindDirectChildrenOnly);
		if (running_task) {
			if (running_task->checkType() == CheckLogTask::CheckType::TrimDirtyLogOnly || running_task->checkType() == CheckLogTask::CheckType::ReplaceDirtyLog) {
				return;
			}
			SHV_EXCEPTION("Device " + shv_path.toStdString() + " is currently sanitized.");
		}
		else {
			shvMessage() << "trim dirty log for" << shv_path;
			CheckLogTask *task = new CheckLogTask(shv_path, CheckLogTask::CheckType::TrimDirtyLogOnly, this);
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
	QTimer *timer = new QTimer(this);
	timer->setInterval(60000);  //let device to collect snapshot
	timer->setSingleShot(true);
	connect(timer, &QTimer::timeout, this, [this, shv_path]() {
		sanitizeLogCache(shv_path, CheckLogTask::CheckType::ReplaceDirtyLog);
		m_newDeviceTimers[shv_path]->deleteLater();
		m_newDeviceTimers.remove(shv_path);
	});
	m_newDeviceTimers[shv_path] = new QTimer(this);
	timer->start();
	setupTimer();
}

void LogSanitizer::onDeviceDisappeared(const QString &shv_path)
{
	setupTimer();
	if (m_newDeviceTimers.contains(shv_path)) {
		m_newDeviceTimers[shv_path]->stop();
		m_newDeviceTimers[shv_path]->deleteLater();
		m_newDeviceTimers.remove(shv_path);
	}
}

void LogSanitizer::sanitizeLogCache()
{
	if (Application::instance()->deviceConnection()->state() != shv::iotqt::rpc::ClientConnection::State::BrokerConnected) {
		return;
	}
	if (findChildren<CheckLogTask*>(QString(), Qt::FindChildOption::FindDirectChildrenOnly).count() > 0) {
		return;
	}
	QStringList online_devices = Application::instance()->deviceMonitor()->onlineDevices();
	for (int i = 0; i < online_devices.count(); ++i) {
		if (Application::instance()->deviceMonitor()->isPushLogDevice(online_devices[i])) {
			online_devices.removeAt(i--);
		}
	}
	if (online_devices.count() == 0) {
		return;
	}
	if (++m_lastCheckedDevice >= online_devices.count()) {
		m_lastCheckedDevice = 0;
	}
	sanitizeLogCache(online_devices[m_lastCheckedDevice], CheckLogTask::CheckType::CheckDirtyLogState);
}

bool LogSanitizer::sanitizeLogCache(const QString &shv_path, CheckLogTask::CheckType check_type)
{
	if (Application::instance()->deviceConnection()->state() != shv::iotqt::rpc::ClientConnection::State::BrokerConnected) {
		return false;
	}
	if (findChild<CheckLogTask*>(shv_path, Qt::FindChildOption::FindDirectChildrenOnly)) {
		return false;
	}
	shvMessage() << "checking logs for" << shv_path;
	CheckLogTask *task = new CheckLogTask(shv_path, check_type, this);
	task->setObjectName(shv_path);
	connect(task, &CheckLogTask::finished, [shv_path, task](bool success) {
		task->setParent(nullptr);
		task->deleteLater();
		shvMessage() << "checking logs for" << shv_path << (success ? "succesfully finished" : "finished with error");
	});
	task->exec();
	return true;
}
