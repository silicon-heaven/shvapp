#include "shv2mqttapp.h"
#include "appclioptions.h"

#include <shv/iotqt/rpc/rpccall.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/node/localfsnode.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/tunnelctl.h>
#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/cponreader.h>

#include <shv/core/stringview.h>

#include <QDirIterator>
#include <QTimer>
#include <QtMqtt/QtMqtt>

using namespace std;
namespace cp = shv::chainpack;
namespace si = shv::iotqt;

#define mqttDebug() shvCDebug("mqtt")
#define mqttInfo() shvCInfo("mqtt")
#define mqttError() shvCError("mqtt")

namespace {
const auto METH_GET_VERSION = "version";
const auto METH_GIT_COMMIT = "gitCommit";
const auto METH_UPTIME = "uptime";
const std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
	{METH_GET_VERSION, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{METH_GIT_COMMIT, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{METH_UPTIME, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ },
};
}


size_t AppRootNode::methodCount(const StringViewList& shv_path)
{
	if (shv_path.empty()) {
		return meta_methods.size();
	}
	return 0;
}

const cp::MetaMethod* AppRootNode::metaMethod(const StringViewList& shv_path, size_t ix)
{
	if (shv_path.empty()) {
		if (meta_methods.size() <= ix) {
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()));
		}
		return &(meta_methods[ix]);
	}
	return nullptr;
}

cp::RpcValue AppRootNode::callMethod(const StringViewList& shv_path, const std::string& method, const cp::RpcValue& params, const cp::RpcValue& user_id)
{
	if (shv_path.empty()) {
		if (method == cp::Rpc::METH_APP_NAME) {
			return QCoreApplication::applicationName().toStdString();
		}

		if (method == METH_GET_VERSION) {
			return QCoreApplication::applicationVersion().toStdString();
		}

		if (method == METH_UPTIME) {
			return Shv2MqttApp::instance()->uptime().toStdString();
		}

		if(method == METH_GIT_COMMIT) {
#ifdef GIT_COMMIT
			return SHV_EXPAND_AND_QUOTE(GIT_COMMIT);
#else
			return "N/A";
#endif
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

cp::RpcValue AppRootNode::callMethodRq(const cp::RpcRequest& rq)
{
	if (rq.shvPath().asString().empty()) {
		if (rq.method() == cp::Rpc::METH_DEVICE_ID) {
			Shv2MqttApp* app = Shv2MqttApp::instance();
			const cp::RpcValue::Map& opts = app->rpcConnection()->connectionOptions().asMap();
			const cp::RpcValue::Map& dev = opts.value(cp::Rpc::KEY_DEVICE).asMap();
			return dev.value(cp::Rpc::KEY_DEVICE_ID).toString();
		}

		if (rq.method() == cp::Rpc::METH_DEVICE_TYPE) {
			return "shv2mqtt";
		}

	}
	return Super::callMethodRq(rq);
}

namespace {
QString client_state_to_string(const QMqttClient::ClientState &client_state)
{
	switch (client_state) {
	case QMqttClient::ClientState::Disconnected: return "Disconnected";
	case QMqttClient::ClientState::Connecting: return "Connecting";
	case QMqttClient::ClientState::Connected: return "Connected";
	}
	return "Invalid enum error";
}

QString client_error_to_string(const QMqttClient::ClientError &client_error)
{
	switch (client_error) {
	case QMqttClient::ClientError::NoError: return "No Error";
	case QMqttClient::ClientError::InvalidProtocolVersion: return "Invalid Protocol Version";
	case QMqttClient::ClientError::IdRejected: return "Id Rejected";
	case QMqttClient::ClientError::ServerUnavailable: return "Server Unavailable";
	case QMqttClient::ClientError::BadUsernameOrPassword: return "Bad Username Or Password";
	case QMqttClient::ClientError::NotAuthorized: return "Not Authorized";
	case QMqttClient::ClientError::TransportInvalid: return "Transport Invalid";
	case QMqttClient::ClientError::ProtocolViolation: return "Protocol Violation";
	case QMqttClient::ClientError::UnknownError: return "Unknown Error";
	case QMqttClient::ClientError::Mqtt5SpecificError: return "Mqtt5 Specific Error";
	}
	return "Invalid enum error";
}
}

Shv2MqttApp::Shv2MqttApp(int& argc, char** argv, AppCliOptions* cli_opts, shv::iotqt::rpc::DeviceConnection* rpc_connection)
	: Super(argc, argv)
	  , m_rpcConnection(rpc_connection)
	  , m_cliOptions(cli_opts)
	  , m_rootTopic(QString::fromStdString(m_cliOptions->mqttRootTopic()))
{
	m_rpcConnection->setParent(this);

	m_rpcConnection->setCliOptions(cli_opts);
	shvInfo() << "Will subscribe to:" << cli_opts->subscribePaths().toCpon();

	connect(m_rpcConnection, &si::rpc::ClientConnection::brokerConnectedChanged, this, &Shv2MqttApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &si::rpc::ClientConnection::rpcMessageReceived, this, &Shv2MqttApp::onRpcMessageReceived);
	m_root = new AppRootNode();
	m_shvTree = new si::node::ShvNodeTree(m_root, this);
	connect(m_shvTree->root(), &si::node::ShvRootNode::sendRpcMessage, m_rpcConnection, &si::rpc::ClientConnection::sendMessage);

	m_mqttClient = new QMqttClient(this);
    m_mqttClient->setHostname(QString::fromStdString(m_cliOptions->mqttHostname()));
    m_mqttClient->setPort(static_cast<quint16>(m_cliOptions->mqttPort()));
	m_mqttClient->setUsername(QString::fromStdString(m_cliOptions->mqttUser()));
	m_mqttClient->setPassword(QString::fromStdString(m_cliOptions->mqttPassword()));
	m_mqttClient->setClientId(QString::fromStdString(m_cliOptions->mqttClientId()));
	const QString version = QString::fromStdString(m_cliOptions->mqttVersion());
	if (version == "mqtt31") {
		m_mqttClient->setProtocolVersion(QMqttClient::MQTT_3_1);
	}
	else if (version == "mqtt311") {
		m_mqttClient->setProtocolVersion(QMqttClient::MQTT_3_1_1);
	}
	else if (version == "mqtt5") {
		m_mqttClient->setProtocolVersion(QMqttClient::MQTT_5_0);
	} else {
		SHV_EXCEPTION("Invalid protocol version specified");
	}

	m_mqttClient->setAutoKeepAlive(true);

	connect(m_mqttClient, &QMqttClient::connected, [] {
		shvInfo() << "Connected to the MQTT broker";
	});
	connect(m_mqttClient, &QMqttClient::disconnected, [] {
		shvWarning() << "Disconnected from the MQTT broker";
	});

	connect(m_mqttClient, &QMqttClient::stateChanged, this,  [] (const QMqttClient::ClientState& s) {
		mqttInfo() << "MQTT state changed:" << client_state_to_string(s);
	});
	connect(m_mqttClient, &QMqttClient::errorChanged, this, [] (const QMqttClient::ClientError& s) {
		mqttError() << "MQTT error occured:" << client_error_to_string(s);
		Shv2MqttApp::exit(1);
	});

	m_mqttClient->connectToHost();
	QTimer::singleShot(0, m_rpcConnection, &si::rpc::ClientConnection::open);

	m_uptime.start();
}

Shv2MqttApp::~Shv2MqttApp()
{
	shvInfo() << "destroying shv2mqtt application";
}

Shv2MqttApp* Shv2MqttApp::instance()
{
	return qobject_cast<Shv2MqttApp*>(QCoreApplication::instance());
}

void Shv2MqttApp::onBrokerConnectedChanged(bool is_connected)
{
	m_isBrokerConnected = is_connected;
	for (const auto& path : m_cliOptions->subscribePaths().asList()) {
		shvInfo() << "Subscribing to" << path;
		m_rpcConnection->callMethodSubscribe(path.toString(), "chng");
	}
}

void Shv2MqttApp::onRpcMessageReceived(const cp::RpcMessage& msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if (msg.isRequest()) {
		cp::RpcRequest rq(msg);
		shvDebug() << "RPC request received:" << rq.toPrettyString();
		if (m_shvTree->root()) {
			m_shvTree->root()->handleRpcRequest(rq);
		}
	} else if (msg.isResponse()) {
		cp::RpcResponse rp(msg);
		shvDebug() << "RPC response received:" << rp.toPrettyString();
	} else if (msg.isSignal()) {
		cp::RpcSignal nt(msg);
		if (m_mqttClient->state() == QMqttClient::Connected) {
			auto topic = shv::coreqt::utils::joinPath(m_rootTopic, nt.shvPath().toStdString());
			auto data = shv::coreqt::utils::jsonValueToByteArray(shv::coreqt::utils::rpcValueToJson(nt.params()));
			mqttInfo() << "published: topic:" << topic << "data:" << data.toStdString();
			m_mqttClient->publish(topic, data);
		}
		shvDebug() << "RPC notify received:" << nt.toPrettyString();
	}
}

QString Shv2MqttApp::uptime() const
{
	qint64 elapsed = m_uptime.elapsed();
	int ms = static_cast<int>(elapsed) % 1000;
	elapsed /= 1000;
	int sec = static_cast<int>(elapsed) % 60;
	elapsed /= 60;
	int min = static_cast<int>(elapsed) % 60;
	elapsed /= 60;
	int hour = static_cast<int>(elapsed) % 24;
	int day = static_cast<int>(elapsed) / 24;
	return QString("%1 day(s) %2:%3:%4.%5")
		.arg(day)
		.arg(hour, 2, 10, QChar('0'))
		.arg(min, 2, 10, QChar('0'))
		.arg(sec, 2, 10, QChar('0'))
		.arg(ms, 3, 10, QChar('0'));
}
