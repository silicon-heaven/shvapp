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
	connect(app->deviceConnection(), &shv::iotqt::rpc::DeviceConnection::stateChanged, this, &LogSanitizer::onShvStateChanged);

	DeviceMonitor *monitor = app->deviceMonitor();
	connect(monitor, &DeviceMonitor::deviceConnectedToBroker, this, &LogSanitizer::onDeviceAppeared);
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
	checkLogs(shv_path, CheckLogType::ReplaceDirtyLog);
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
	checkLogs(online_devices[m_lastCheckedDevice], CheckLogType::CheckDirtyLogState);
}

void LogSanitizer::checkLogs(const QString &shv_path, CheckLogType check_type)
{
	if (findChild<CheckLogTask*>(shv_path, Qt::FindChildOption::FindDirectChildrenOnly)) {
		return;
	}
	m_timer.stop();
	shvInfo() << "checking logs for" << shv_path;
	CheckLogTask *task = new CheckLogTask(shv_path, check_type, this);
	task->setObjectName(shv_path);
	connect(task, &CheckLogTask::finished, [this, shv_path, task](bool success) {
		task->deleteLater();
		shvInfo() << "checking logs for" << shv_path << (success ? "succesfully finished" : "finished with error");
	});
	connect(task, &CheckLogTask::destroyed, [this]() {
		if (findChildren<CheckLogTask*>(QString(), Qt::FindChildOption::FindDirectChildrenOnly).count() == 0 &&
			Application::instance()->deviceConnection()->state() == shv::iotqt::rpc::ClientConnection::State::BrokerConnected) {
			m_timer.start();
		}
	});
	task->exec();
}
