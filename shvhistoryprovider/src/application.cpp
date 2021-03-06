#include "application.h"
#include "appclioptions.h"
#include "devicemonitor.h"
#include "dirtylogmanager.h"
#include "logsanitizer.h"
#include "rootnode.h"

#include <shv/coreqt/log.h>
#include <shv/coreqt/exception.h>
#include <shv/iotqt/rpc/rpc.h>

#include <QTimer>

namespace cp = shv::chainpack;

const int Application::CHUNK_RECORD_COUNT = 10000;
const char *Application::DIRTY_LOG_NODE = "dirtyLog";
const char *Application::START_TS_NODE = "startTS";

Application::Application(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
	, m_root(nullptr)
{
	if (!cli_opts->deviceId_isset()) {
		cli_opts->setDeviceId("shvhistoryprovider");
	}

	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);
	if (!cli_opts->serverHost_isset()) {
        cli_opts->setServerHost("localhost");
    }
    if (!cli_opts->user_isset()) {
        cli_opts->setUser("iot");
    }
    if (!cli_opts->password_isset()) {
        cli_opts->setPassword("lub42DUB");
		cli_opts->setLoginType("PLAIN");
    }
	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::stateChanged, this, &Application::onShvStateChanged);

	m_uptime.start();
	connectToShv();

	m_deviceMonitor = new DeviceMonitor(this);

	QEventLoop loop(this);
	connect(m_deviceMonitor, &DeviceMonitor::sitesDownloadFinished, &loop, &QEventLoop::quit);
	loop.exec();

	m_logSanitizer = new LogSanitizer(this);
	m_dirtyLogManager = new DirtyLogManager(this);
}

Application::~Application()
{
	shvInfo() << "destroying" << QCoreApplication::applicationName() << "application";
}

shv::iotqt::rpc::DeviceConnection *Application::deviceConnection()
{
	return m_rpcConnection;
}

void Application::connectToShv()
{
	shvInfo() << "Connecting to SHV Broker";
	m_rpcConnection->open();
}

void Application::disconnectFromShv()
{
	if (m_rpcConnection->state() != shv::iotqt::rpc::DeviceConnection::State::BrokerConnected) {
		return;
	}
	m_rpcConnection->close();
}

void Application::quit()
{
	if (m_rpcConnection->state() == shv::iotqt::rpc::DeviceConnection::State::BrokerConnected) {
		connect(m_rpcConnection, &shv::iotqt::rpc::DeviceConnection::stateChanged, [this]() {
			Super::quit();
		});
		m_rpcConnection->close();
	}
	else {
		Super::quit();
	}
}

Application *Application::instance()
{
	return qobject_cast<Application *>(QCoreApplication::instance());
}

QString Application::uptime() const
{
	qint64 elapsed = m_uptime.elapsed();
	int ms = elapsed % 1000;
	elapsed /= 1000;
	int sec = elapsed % 60;
	elapsed /= 60;
	int min = elapsed % 60;
	elapsed /= 60;
	int hour = elapsed % 60;
	int day = (int)elapsed / 60;
	return QString("%1 day(s) %2:%3:%4.%5")
			.arg(day)
			.arg(hour, 2, 10, QChar('0'))
			.arg(min, 2, 10, QChar('0'))
			.arg(sec, 2, 10, QChar('0'))
			.arg(ms, 3, 10, QChar('0'));
}

void Application::onShvStateChanged()
{
	if (m_rpcConnection->state() == shv::iotqt::rpc::ClientConnection::State::BrokerConnected) {
		shvInfo() << "SHV Broker connected";
		if (!m_root) {
			m_root = new RootNode();
			m_shvTree = new shv::iotqt::node::ShvNodeTree(m_root, this);
			connect(m_shvTree->root(), &shv::iotqt::node::ShvRootNode::sendRpcMessage, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::sendMessage);
			connect(m_rpcConnection, &shv::iotqt::rpc::DeviceConnection::rpcMessageReceived, m_root, &RootNode::onRpcMessageReceived);
		}
	}
	else if (m_rpcConnection->state() == shv::iotqt::rpc::ClientConnection::State::NotConnected) {
		shvInfo() << "SHV Broker disconnected";
	}
}
