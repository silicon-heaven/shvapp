#include "application.h"
#include "appclioptions.h"
#include "rootnode.h"
#include "devicemonitor.h"
#include "logsanitizer.h"
#include "migration.h"
#include "dirtylogmanager.h"

#include <shv/coreqt/log.h>
#include <shv/coreqt/exception.h>
#include <shv/iotqt/rpc/rpc.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>

#include <QTimer>

namespace cp = shv::chainpack;

const int Application::SINGLE_FILE_RECORD_COUNT = 10000;

Application::Application(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
	, m_root(nullptr)
{
	if (!cli_opts->deviceId_isset()) {
		cli_opts->setDeviceId("shvhistoryprovider");
	}
	m_masterBrokerPath = QString::fromStdString(cli_opts->masterBrokerPath());

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

//	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvClient::onRpcMessageReceived);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::stateChanged, this, &Application::onShvStateChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::stateChanged, this, &Application::shvStateChanged);

	m_uptime.start();
	connectToShv();

	m_deviceMonitor = new DeviceMonitor(this);
	Migration migration;
	if (migration.isNeeded()) {
		QMetaObject::Connection *connection = new QMetaObject::Connection;
		*connection = connect(m_deviceMonitor, &DeviceMonitor::deviceScanFinished, [this, connection]() {
			disconnect(*connection);
			delete connection;
			disconnectFromShv();
			Migration().exec();
			connectToShv();
			m_logSanitizer = new LogSanitizer(this);
			m_dirtyLogManager = new DirtyLogManager(this);
		});
	}
	else {
		m_logSanitizer = new LogSanitizer(this);
		m_dirtyLogManager = new DirtyLogManager(this);
	}
}

Application::~Application()
{
	shvInfo() << "destroying" << QCoreApplication::applicationName() << "application";
}

void Application::shvCall(const QString &shv_path, const QString &method, shv::iotqt::rpc::RpcResponseCallBack::CallBackFunction callback)
{
	shvCall(shv_path, method, cp::RpcValue(), callback);
}

void Application::shvCall(const QString &shv_path, const QString &method, const shv::chainpack::RpcValue &params, shv::iotqt::rpc::RpcResponseCallBack::CallBackFunction callback)
{
	int rq_id = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rq_id, this);
	cb->start([callback](const cp::RpcResponse &resp) {
		callback(resp);
	});
	m_rpcConnection->callShvMethod(rq_id, shv_path.toStdString(), method.toStdString(), params);
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

QString Application::elesysPath() const
{
	return masterBrokerPath() + QString::fromStdString(m_cliOptions->elesysPath());
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
