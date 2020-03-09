#include "application.h"
#include "checklogtask.h"
#include "devicemonitor.h"
#include "logsanitizer.h"

#include <shv/coreqt/log.h>

LogSanitizer::LogSanitizer(QObject *parent)
	: QObject(parent)
	, m_lastCheckedDevice(-1)
	, m_timer(this)
{
	m_timer.setSingleShot(true);
	m_timer.setInterval(60 * 1000);
	connect(&m_timer, &QTimer::timeout, this, qOverload<>(&LogSanitizer::checkLogs));

	Application *app = Application::instance();
	auto *conn = app->deviceConnection();
	connect(conn, &shv::iotqt::rpc::DeviceConnection::stateChanged, this, &LogSanitizer::onShvStateChanged);

	DeviceMonitor *monitor = app->deviceMonitor();
	connect(monitor, &DeviceMonitor::deviceConnectedToBroker, this, &LogSanitizer::onDeviceAppeared);

	if (conn->state() == shv::iotqt::rpc::DeviceConnection::State::BrokerConnected) {
		onShvStateChanged(conn->state());
	}
}

void LogSanitizer::trimDirtyLog(const QString &shv_path)
{
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

void LogSanitizer::onShvStateChanged(shv::iotqt::rpc::ClientConnection::State state)
{
	if (state == shv::iotqt::rpc::ClientConnection::State::BrokerConnected) {
		m_timer.start();
	}
	else {
		m_timer.stop();
		m_lastCheckedDevice = -1;
	}
}

void LogSanitizer::onDeviceAppeared(const QString &shv_path)
{
	checkLogs(shv_path, CheckLogTask::CheckType::ReplaceDirtyLog);
}

void LogSanitizer::checkLogs()
{
	const QStringList &online_devices = Application::instance()->deviceMonitor()->onlineDevices();
	if (online_devices.count() == 0) {
		return;
	}
	if (++m_lastCheckedDevice >= online_devices.count()) {
		m_lastCheckedDevice = 0;
	}
	checkLogs(online_devices[m_lastCheckedDevice], CheckLogTask::CheckType::CheckDirtyLogState);
}

void LogSanitizer::checkLogs(const QString &shv_path, CheckLogTask::CheckType check_type)
{
	if (findChild<CheckLogTask*>(shv_path, Qt::FindChildOption::FindDirectChildrenOnly)) {
		return;
	}
	m_timer.stop();
	shvMessage() << "checking logs for" << shv_path;
	CheckLogTask *task = new CheckLogTask(shv_path, check_type, this);
	task->setObjectName(shv_path);
	connect(task, &CheckLogTask::finished, [shv_path, task](bool success) {
		task->setParent(nullptr);
		task->deleteLater();
		shvMessage() << "checking logs for" << shv_path << (success ? "succesfully finished" : "finished with error");
	});
	connect(task, &CheckLogTask::destroyed, [this]() {
		if (findChildren<CheckLogTask*>(QString(), Qt::FindChildOption::FindDirectChildrenOnly).count() == 0 &&
			Application::instance()->deviceConnection()->state() == shv::iotqt::rpc::ClientConnection::State::BrokerConnected) {
			m_timer.start();
		}
	});
	task->exec();
}
