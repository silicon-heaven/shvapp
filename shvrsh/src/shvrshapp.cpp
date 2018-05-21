#include "shvrshapp.h"
#include "appclioptions.h"

#include <shv/iotqt/rpc/tunnelconnection.h>
#include <shv/iotqt/rpc/tunnelhandle.h>
#include <shv/iotqt/node/shvnodetree.h>

#include <shv/coreqt/log.h>
#include <shv/chainpack/metamethod.h>
#include <shv/core/stringview.h>
#include <shv/core/utils.h>

#include <QSocketNotifier>
#include <QTimer>

#include <iostream>

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>

namespace cp = shv::chainpack;
namespace ir = shv::iotqt::rpc;

//const char METH_WRITE[] = "write";

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, false},
	{cp::Rpc::METH_CONNECTION_TYPE, cp::MetaMethod::Signature::RetVoid, false},
	//{cp::Rpc::KEY_TUNNEL_HANDLE, cp::MetaMethod::Signature::RetVoid, false},
	//{METH_WRITE, cp::MetaMethod::Signature::RetParam, false},
};

size_t AppRootNode::methodCount()
{
	return meta_methods.size();
}

const cp::MetaMethod *AppRootNode::metaMethod(size_t ix)
{
	if(meta_methods.size() <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()));
	return &(meta_methods[ix]);
}

shv::chainpack::RpcValue AppRootNode::call(const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(method == cp::Rpc::METH_APP_NAME) {
		return QCoreApplication::instance()->applicationName().toStdString();
	}
	if(method == cp::Rpc::METH_CONNECTION_TYPE) {
		return ShvRshApp::instance()->rpcConnection()->connectionType();
	}
	//if(method == cp::Rpc::KEY_TUNNEL_HANDLE) {
	//	return ShvRExecApp::instance()->rpcConnection()->tunnelHandle();
	//}
	/*
	if(method == METH_WRITE) {
		const shv::chainpack::RpcValue::Blob data = params.toBlob();
		qint64 len = ShvRshApp::instance()->writeProcessStdin(data.data(), data.size());
		if(len < 0)
			shvError() << "Error writing process stdin.";
		return cp::RpcValue{};
	}
	*/
	return Super::call(method, params);
}

ShvRshApp::ShvRshApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	m_rpcConnection = new shv::iotqt::rpc::ClientConnection(this);

	if(!cli_opts->user_isset())
		cli_opts->setUser("iot");
	if(!cli_opts->password_isset())
		cli_opts->setPassword("lub42DUB");

	cli_opts->setHeartbeatInterval(0);
	cli_opts->setReconnectInterval(0);
	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::socketConnectedChanged, [](bool connected) {
		if(!connected) {
			shvError() << "Socket disconnected, quitting";
			quit();
		}
	});
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &ShvRshApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvRshApp::onRpcMessageReceived);

	AppRootNode *root = new AppRootNode();
	m_shvTree = new shv::iotqt::node::ShvNodeTree(root, this);

	fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
	QSocketNotifier *stdin_notifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
	connect(stdin_notifier, &QSocketNotifier::activated, this, &ShvRshApp::onReadyReadStdIn);

	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
}

ShvRshApp::~ShvRshApp()
{
	shvInfo() << "destroying shv agent application";
	disconnect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::socketConnectedChanged, 0, 0);
}

ShvRshApp *ShvRshApp::instance()
{
	return qobject_cast<ShvRshApp*>(QCoreApplication::instance());
}

void ShvRshApp::onBrokerConnectedChanged(bool is_connected)
{
	if(is_connected) {
		std::string tunp = m_cliOptions->tunnelShvPath().toStdString();
		std::string tunm = m_cliOptions->tunnelMethod().toStdString();
		if(!tunp.empty()) {
			m_launchRexecRequestId = m_rpcConnection->callShvMethod(tunp, tunm);
			shvDebug() << "opening tunnel on path:" << tunp << "method:" << tunm << "request id:" << m_tunnelRequestId;
		}
		else {
			shvError() << "Invalid tunnel path:" << tunp << "method:" << tunm;
			quit();
		}
	}
	else {
		/// once connection to tunnel is lost, tunnel handle becomes invalid, destroy tunnel
		quit();
	}
}

void ShvRshApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		cp::RpcResponse resp = cp::RpcResponse::forRequest(rq.metaData());
		try {
			//shvInfo() << "RPC request received:" << rq.toCpon();
			const cp::RpcValue shv_path = rq.shvPath();
			std::string path_rest;
			shv::iotqt::node::ShvNode *nd = m_shvTree->cd(shv_path.toString(), &path_rest);
			if(!nd)
				SHV_EXCEPTION("Path not found: " + shv_path.toString());
			rq.setShvPath(path_rest);
			resp.setResult(nd->processRpcRequest(rq));
		}
		catch (shv::core::Exception &e) {
			resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodInvocationException, e.message()));
		}
		m_rpcConnection->sendMessage(resp);
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rsp(msg);
		shvInfo() << "RPC response received request id:" << rsp.requestId().toCpon() << rsp.toPrettyString();
		//shvInfo() << __LINE__ << m_tunnelRequestId << m_tunnelShvPath;
		//shv::chainpack::RpcValue v = rsp.requestId();
		if(rsp.requestId() == m_launchRexecRequestId) {
			if(rsp.isError()) {
				shvError() << "Open tunnel error:" << rsp.error().toString();
				quit();
				return;
			}
			const cp::RpcValue::Map &result = rsp.result().toMap();
			if(result.empty()) {
				shvError() << "Launch rexec error, shv path should be received:" << rsp.toCpon();
				quit();
				return;
			}
			std::string relative_path = shv::core::Utils::joinPath(m_cliOptions->tunnelShvPath().toStdString(), result.value(cp::Rpc::KEY_MOUT_POINT).toString());
			m_tunnelShvPath = shv::core::Utils::simplifyPath(relative_path);
			shvInfo() << "tunnel shv path:" << m_tunnelShvPath;
			launchRemoteShellProcess();
		}
		else if(rsp.requestId() == m_tunnelRequestId) {
			//shvInfo() << __LINE__;
			//shvInfo() << rsp.result().toCpon();
			shv::chainpack::RpcValue result = rsp.result();
			if(m_writeTunnelHandle.empty()) {
				if(result.metaData().metaTypeId() != shv::iotqt::rpc::TunnelHandle::MetaType::ID) {
					shvError() << "TunnelHandle should be received:" << rsp.toCpon();
					quit();
					return;
				}
				m_writeTunnelHandle = shv::iotqt::rpc::TunnelHandle(result.toIMap());
			}
			else {
				const shv::chainpack::RpcValue::List &lst = rsp.result().toList();
				int channel = lst.value(0).toInt();
				if(channel == STDOUT_FILENO || channel == STDERR_FILENO) {
					const shv::chainpack::RpcValue::Blob &blob = lst.value(1).toBlob();
					ssize_t n = ::write(channel, blob.data(), blob.size());
					if(n < (ssize_t)blob.size())
						shvError() << "Error write remote data to stdout";
				}
				else {
					shvError() << "Ignoring invalid channel number data received, channel:" << channel;
				}
			}
		}
	}
	else if(msg.isNotify()) {
		cp::RpcNotify ntf(msg);
		shvInfo() << "RPC notify received:" << ntf.toCpon();
		if(ntf.method() == cp::Rpc::NTF_CONNECTED_CHANGED) {
			quit();
		}
	}
}

void ShvRshApp::onReadyReadStdIn()
{
	std::vector<char> data = shv::core::Utils::readAllFd(STDIN_FILENO);
	writeToTunnel(0, cp::RpcValue::Blob(data.data(), data.size()));
}

void ShvRshApp::writeToTunnel(int channel, const cp::RpcValue &data)
{
	//shvInfo() << m_rpcConnection->isBrokerConnected() << m_tunnelShvPath << "GGG";
	if(!m_rpcConnection->isBrokerConnected() || m_writeTunnelHandle.empty()) {
		shvError() << "Tunnel not open, throwing away data:" << data.toCpon();
		//quit();
	}
	else {
		cp::RpcResponse resp;
		resp.setRequestId(m_writeTunnelHandle.requestId());
		resp.setCallerIds(m_writeTunnelHandle.callerIds());
		cp::RpcValue::List result{channel, data};
		resp.setResult(result);
		m_rpcConnection->sendMessage(resp);
	}
}

void ShvRshApp::launchRemoteShellProcess()
{
	struct winsize ws;
	if (::ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) < 0)
		SHV_EXCEPTION("ioctl-TIOCGWINSZ");
	m_rpcConnection->createSubscription(m_tunnelShvPath, cp::Rpc::NTF_CONNECTED_CHANGED);
	{
		cp::RpcRequest rq;
		rq.setShvPath(m_tunnelShvPath);
		rq.setMethod("runPtyCmd");
		rq.setParams(cp::RpcValue::List{"/bin/sh", ws.ws_col, ws.ws_row});
		rq.setOpenTunnelFlag();
		m_tunnelRequestId = m_rpcConnection->callMethod(rq);
	}
}

