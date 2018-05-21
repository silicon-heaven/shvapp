#pragma once

#include <shv/iotqt/node/shvnode.h>

#include <QCoreApplication>

class AppCliOptions;

namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}
namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}

class AppRootNode : public shv::iotqt::node::ShvRootNode
{
	using Super = shv::iotqt::node::ShvRootNode;
public:
	explicit AppRootNode(QObject *parent = nullptr) : Super(parent) {}

	size_t methodCount() override;
	const shv::chainpack::MetaMethod* metaMethod(size_t ix) override;

	shv::chainpack::RpcValue call(const std::string &method, const shv::chainpack::RpcValue &params) override;

	shv::chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq) override;
};

class ShvAgentApp : public QCoreApplication
{
	Q_OBJECT
private:
	using Super = QCoreApplication;
public:
	ShvAgentApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~ShvAgentApp() Q_DECL_OVERRIDE;

	static ShvAgentApp *instance();
	shv::iotqt::rpc::DeviceConnection *rpcConnection() const {return m_rpcConnection;}

	void launchRexec(const shv::chainpack::RpcRequest &rq);
	void runCmd(const shv::chainpack::RpcRequest &rq);
private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	//void onRootNodeSendRpcMesage(const shv::chainpack::RpcMessage &msg);
private:
	shv::iotqt::rpc::DeviceConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;

	shv::iotqt::node::ShvNodeTree *m_shvTree = nullptr;
#ifdef HANDLE_UNIX_SIGNALS
	Q_SIGNAL void aboutToTerminate(int sig);

	void installUnixSignalHandlers();
	Q_SLOT void handleUnixSignal();
#endif
};

