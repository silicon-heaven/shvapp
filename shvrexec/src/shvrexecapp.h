#pragma once

#include <shv/iotqt/node/shvnode.h>
#include <shv/iotqt/rpc/tunnelhandle.h>

#include <shv/chainpack/rpcmessage.h>

#include <QCoreApplication>

class PtyProcess;
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

	size_t methodCount() override;
	const shv::chainpack::MetaMethod* metaMethod(size_t ix) override;

	shv::chainpack::RpcValue call(const std::string &method, const shv::chainpack::RpcValue &params) override;
	shv::chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq) override;
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

	shv::chainpack::RpcValue runCmd(const shv::chainpack::RpcRequest &rq);
	shv::chainpack::RpcValue runPtyCmd(const shv::chainpack::RpcRequest &rq);

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

	void sendProcessOutput(int channel, const char *data, size_t data_len);
	void closeAndQuit();
private:
	shv::iotqt::rpc::TunnelConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;

	shv::iotqt::node::ShvNodeTree *m_shvTree = nullptr;

	shv::iotqt::rpc::TunnelHandle m_writeTunnelHandle;
	unsigned m_readTunnelRequestId = 0;

	PtyProcess *m_ptyCmdProc = nullptr;
	QProcess *m_cmdProc = nullptr;
	//int m_termWidth = 0;
	//int m_termHeight = 0;
};

