#pragma once

#include <shv/iotqt/node/shvnode.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QtMqtt/QMqttClient>

class AppCliOptions;

namespace shv::chainpack { class RpcMessage; }
namespace shv::iotqt::rpc { class DeviceConnection; }
namespace shv::iotqt::node { class ShvNodeTree; }

class AppRootNode : public shv::iotqt::node::ShvRootNode
{
	using Super = shv::iotqt::node::ShvRootNode;
public:
	explicit AppRootNode(QObject* parent = nullptr) : Super(parent) {}

	size_t methodCount(const StringViewList& shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList& shv_path, size_t ix) override;

	shv::chainpack::RpcValue callMethodRq(const shv::chainpack::RpcRequest& rq) override;
	shv::chainpack::RpcValue callMethod(const StringViewList& shv_path, const std::string& method, const shv::chainpack::RpcValue& params, const shv::chainpack::RpcValue& user_id) override;
};

class Shv2MqttApp : public QCoreApplication
{
	Q_OBJECT
private:
	using Super = QCoreApplication;
public:
	Shv2MqttApp(int& argc, char** argv, AppCliOptions* cli_opts, shv::iotqt::rpc::DeviceConnection* rpc_connection);
	~Shv2MqttApp() Q_DECL_OVERRIDE;

	static Shv2MqttApp* instance();
	shv::iotqt::rpc::DeviceConnection* rpcConnection() const {return m_rpcConnection;}
	AppCliOptions* cliOptions() {return m_cliOptions;}
	QString uptime() const;

private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage& msg);

private:
	bool m_isBrokerConnected = false;
	QElapsedTimer m_uptime;
	shv::iotqt::rpc::DeviceConnection* m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;
	shv::iotqt::node::ShvNodeTree* m_shvTree = nullptr;
	AppRootNode* m_root = nullptr;
	QMqttClient* m_mqttClient = nullptr;
};
