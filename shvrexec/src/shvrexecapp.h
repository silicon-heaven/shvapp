#pragma once

#include <shv/iotqt/node/shvnode.h>

#include <QCoreApplication>

class QProcess;
class AppCliOptions;

namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class TunnelConnection; }}}
namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}

class AppRootNode : public shv::iotqt::node::ShvRootNode
{
	using Super = shv::iotqt::node::ShvRootNode;
public:
	explicit AppRootNode(QObject *parent = nullptr) : Super(parent) {}

	shv::chainpack::RpcValue dir(const shv::chainpack::RpcValue &methods_params) override;
	shv::chainpack::RpcValue call(const std::string &method, const shv::chainpack::RpcValue &params) override;
};

class ShvRExecApp : public QCoreApplication
{
	Q_OBJECT
private:
	using Super = QCoreApplication;
public:
	ShvRExecApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~ShvRExecApp() Q_DECL_OVERRIDE;

	static ShvRExecApp* instance();
	shv::iotqt::rpc::TunnelConnection *rpcConnection() const {return m_rpcConnection;}

	qint64 writeProcessStdin(const char *data, size_t len);
private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	void onReadyReadProcessStandardOutput();
	void onReadyReadProcessStandardError();
private:
	shv::iotqt::rpc::TunnelConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;

	shv::iotqt::node::ShvNodeTree *m_shvTree = nullptr;

	shv::chainpack::RpcValue m_tunnelHandle;

	QProcess *m_cmdProc = nullptr;
};

