#pragma once

#include <shv/iotqt/node/shvnode.h>

#include <QCoreApplication>

class AppCliOptions;
class QTimer;

namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}
namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}

class AppRootNode : public shv::iotqt::node::ShvRootNode
{
	using Super = shv::iotqt::node::ShvRootNode;
public:
	explicit AppRootNode(QObject *parent = nullptr) : Super(parent) {}

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	//shv::chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq) override;
	shv::chainpack::RpcValue callMethodRq(const shv::chainpack::RpcRequest &rq) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;
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

	AppCliOptions* cliOptions() {return m_cliOptions;}

	void launchRexec(const shv::chainpack::RpcRequest &rq);
	void runCmd(const shv::chainpack::RpcRequest &rq, bool std_out_only = false);
private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void updateConnStatusFile();

	void tester_processShvCalls();
private:
	shv::iotqt::rpc::DeviceConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;

	shv::iotqt::node::ShvNodeTree *m_shvTree = nullptr;
	//QTimer *m_conStatusFileUpdateTimer = nullptr;
	bool m_isBrokerConnected = false;

	shv::chainpack::RpcValue m_testerScript;
	unsigned m_currentTestIndex = 0;
#ifdef HANDLE_UNIX_SIGNALS
	Q_SIGNAL void aboutToTerminate(int sig);

	void installUnixSignalHandlers();
	Q_SLOT void handleUnixSignal();
#endif
};

