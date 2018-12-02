#pragma once

#include <shv/iotqt/node/shvnode.h>

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/tunnelctl.h>

#include <QCoreApplication>

class PtyProcess;
class QProcess;
class QTimer;
class AppCliOptions;

namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class ClientConnection; }}}
namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}

class AppRootNode : public shv::iotqt::node::ShvRootNode
{
	using Super = shv::iotqt::node::ShvRootNode;
public:
	explicit AppRootNode(QObject *parent = nullptr) : Super(parent) {}

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
	//shv::chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq) override;
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
	shv::iotqt::rpc::ClientConnection *rpcConnection() const {return m_rpcConnection;}

	void runCmd(const shv::chainpack::RpcValue &params);
	void runPtyCmd(const shv::chainpack::RpcValue &params);

	qint64 writeCmdProcessStdIn(const char *data, size_t len);
private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	// PTTY process
	void onReadyReadMasterPty();
	// non  PTTY process
	void onReadyReadProcessStandardOutput();
	void onReadyReadProcessStandardError();

	//void setTerminalWindowSize(int w, int h);
	//void openTunnel(const std::string &method, const shv::chainpack::RpcValue &params, int request_id, const shv::chainpack::RpcValue &cids);

	void sendProcessOutput(int channel, const char *data, size_t data_len);
	void closeAndQuit();
private:
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;

	shv::iotqt::node::ShvNodeTree *m_shvTree = nullptr;

	int m_writeTunnelRequestId = 0;
	shv::chainpack::RpcValue m_writeTunnelCallerIds;
	int m_readTunnelRequestId = 0;

	shv::chainpack::TunnelCtl m_tunnelCtl;
	// which shvrexec shv method will be called after conection to broker
	shv::chainpack::RpcValue m_onConnectedCall;
	PtyProcess *m_ptyCmdProc = nullptr;
	QProcess *m_cmdProc = nullptr;
	//int m_termWidth = 0;
	//int m_termHeight = 0;
	QTimer *m_inactivityTimer = nullptr;
};

