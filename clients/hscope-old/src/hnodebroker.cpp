#include "hnodebroker.h"
#include "hnodeagents.h"
#include "confignode.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/clientappclioptions.h>
#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/core/utils/shvpath.h>

#include <QTimer>

static const char KEY_DISABLED[] = "disabled";
static const char KEY_HOST[] = "host";
static const char KEY_PORT[] = "port";
static const char KEY_USER[] = "user";
static const char KEY_PASSWORD[] = "password";
static const char KEY_LOGIN_TYPE[] = "loginType";

HNodeBroker::HNodeBroker(const std::string &node_id, HNode *parent)
	: Super(node_id, parent)
{
	shvDebug() << "creating:" << metaObject()->className() << node_id;
	//connect(this, &HNode::statusChanged, [this]() { updateOverallStatus(); });
}

void HNodeBroker::load()
{
	Super::load();
	for (const std::string &dir : lsConfigDir()) {
		auto *nd = new HNodeAgents(dir, this);
		nd->load();
	}
	connect(m_configNode, &ConfigNode::configSaved, this, &HNodeBroker::reconnect);
	QTimer::singleShot(0, this, &HNodeBroker::reconnect);
}

shv::iotqt::rpc::ClientConnection *HNodeBroker::rpcConnection()
{
	if(!m_rpcConnection) {
		m_rpcConnection = new shv::iotqt::rpc::ClientConnection(this);

		shv::iotqt::rpc::ClientAppCliOptions opts;
		opts.setServerHost(configValueOnPath(KEY_HOST).asString());
		opts.setServerPort(configValueOnPath(KEY_PORT).toInt());
		opts.setUser(configValueOnPath(KEY_USER).asString());
		opts.setPassword(configValueOnPath(KEY_PASSWORD).asString());
		opts.setLoginType(configValueOnPath(KEY_LOGIN_TYPE).asString());
		m_rpcConnection->setCliOptions(&opts);

		connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &HNodeBroker::brokerConnectedChanged);
		connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &HNodeBroker::rpcMessageReceived);

		connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::stateChanged, this, [this](shv::iotqt::rpc::ClientConnection::State state) {
			shvInfo() << "Agent connection to broker state changed:" << this->shvPath() << (int)state << shv::iotqt::rpc::ClientConnection::stateToString(state);
			switch (state) {
			case shv::iotqt::rpc::ClientConnection::State::NotConnected:
				//setStatus({NodeStatus::Value::Info, "Not connected."});
				break;
			case shv::iotqt::rpc::ClientConnection::State::Connecting:
				//setStatus({NodeStatus::Value::Info, "Connecting to broker."});
				break;
			case shv::iotqt::rpc::ClientConnection::State::SocketConnected:
				//setStatus({NodeStatus::Value::Info, "Logging in to broker."});
				break;
			case shv::iotqt::rpc::ClientConnection::State::BrokerConnected:
				setStatus({NodeStatus::Level::Ok, "Connected to broker."});
				break;
			case shv::iotqt::rpc::ClientConnection::State::ConnectionError:
				setStatus({NodeStatus::Level::Error, "Connect to broker error."});
				break;
			}
		});
		//connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &HScopeApp::onRpcMessageReceived);
	}
	return m_rpcConnection;
}

void HNodeBroker::reconnect()
{
	SHV_SAFE_DELETE(m_rpcConnection);
	shv::iotqt::rpc::ClientConnection *conn = rpcConnection();
	if(!configValueOnPath(KEY_DISABLED).toBool()) {
		shvInfo() << "Broker connection opening ..." << shvPath();
		QTimer::singleShot(0, conn, &shv::iotqt::rpc::ClientConnection::open);
	}
}

