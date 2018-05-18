#pragma once

#include <shv/iotqt/node/shvnode.h>

#include <QCoreApplication>

class AppCliOptions;

namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class ClientConnection; }}}
namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}

class AppRootNode : public shv::iotqt::node::ShvRootNode
{
	using Super = shv::iotqt::node::ShvRootNode;
public:
	explicit AppRootNode(QObject *parent = nullptr) : Super(parent) {}

	size_t methodCount() override;
	const shv::chainpack::MetaMethod* metaMethod(size_t ix) override;

	shv::chainpack::RpcValue call(const std::string &method, const shv::chainpack::RpcValue &params) override;
};

class ShvRshApp : public QCoreApplication
{
	Q_OBJECT
private:
	using Super = QCoreApplication;
public:
	ShvRshApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~ShvRshApp() Q_DECL_OVERRIDE;

	static ShvRshApp* instance();
	shv::iotqt::rpc::ClientConnection *rpcConnection() const {return m_rpcConnection;}

	//qint64 writeProcessStdin(const char *data, size_t len);
private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void onReadyReadStdIn();
	void writeToTunnel(int channel, const shv::chainpack::RpcValue &data);
	void launchRemoteShell();
private:
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;
	//std::string m_tunnelOpenMethodShvPath;
	std::string m_tunnelShvPath;
	unsigned m_createTunnelRequestId = 0;
	unsigned m_tunnelRequestId = 0;

	shv::iotqt::node::ShvNodeTree *m_shvTree = nullptr;
};

