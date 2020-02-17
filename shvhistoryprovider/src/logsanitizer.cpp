#include "application.h"
#include "checklogrequest.h"
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
	connect(app, &Application::shvStateChanged, this, &LogSanitizer::onShvStateChanged);

	DeviceMonitor *monitor = app->deviceMonitor();
	connect(monitor, &DeviceMonitor::deviceAppeared, this, &LogSanitizer::onDeviceAppeared);
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
	checkLogs(shv_path, CheckLogType::OnDeviceAppeared);
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
	checkLogs(online_devices[m_lastCheckedDevice], CheckLogType::Periodic);
}

void LogSanitizer::checkLogs(const QString &shv_path, CheckLogType check_type)
{
	if (m_runningChecks.contains(shv_path)) {
		return;
	}
	m_timer.stop();
	shvInfo() << "checking logs for" << shv_path;
	CheckLogRequest *request = new CheckLogRequest(shv_path, check_type, this);
	m_runningChecks << shv_path;
	connect(request, &CheckLogRequest::finished, [this, shv_path, request]() {
		request->deleteLater();
		shvInfo() << "checking logs for" << shv_path << "finished";
		m_runningChecks.removeOne(shv_path);
		if (m_runningChecks.count() == 0) {
			m_timer.start();
		}
	});
	request->exec();
}
