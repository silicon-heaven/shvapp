#include "shvrshapp.h"
#include "appclioptions.h"

#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/iotqt/node/shvnodetree.h>

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/tunnelctl.h>
#include <shv/coreqt/log.h>
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

#define logTunnelD() nCDebug("Tunnel")

namespace cp = shv::chainpack;
namespace ir = shv::iotqt::rpc;

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, false},
	//{cp::Rpc::METH_CONNECTION_TYPE, cp::MetaMethod::Signature::RetVoid, false},
};

size_t AppRootNode::methodCount(const StringViewList &shv_path)
{
	if(shv_path.empty())
		return meta_methods.size();
	return 0;
}

const cp::MetaMethod *AppRootNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty()) {
		if(meta_methods.size() <= ix)
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()));
		return &(meta_methods[ix]);
	}
	return nullptr;
}

shv::chainpack::RpcValue AppRootNode::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == cp::Rpc::METH_APP_NAME) {
			return QCoreApplication::instance()->applicationName().toStdString();
		}
		//if(method == cp::Rpc::METH_CONNECTION_TYPE) {
		//	return ShvRshApp::instance()->rpcConnection()->connectionType();
		//}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

ShvRshApp::ShvRshApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	//cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	m_rpcConnection = new shv::iotqt::rpc::ClientConnection(this);

	if(!cli_opts->user_isset())
		cli_opts->setUser("iot");
	if(!cli_opts->password_isset())
		cli_opts->setPassword("lub42DUB");

	if(cli_opts->reconnectInterval() > 0) {
		shvInfo() << "Ignoring reconnect interval option, setting it to 0.";
		cli_opts->setReconnectInterval(0);
	}
	if(cli_opts->heartBeatInterval() > 0) {
		shvInfo() << "Ignoring heart-beat interval option, setting it to 0. If set, then shvrsh can be disconnected in case of user inactivity.";
		cli_opts->setHeartBeatInterval(0);
	}
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
	connect(m_shvTree->root(), &shv::iotqt::node::ShvRootNode::sendRpcMessage, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::sendMessage);

	fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
	QSocketNotifier *stdin_notifier = new QSocketNotifier(STDIN_FILENO, QSocketNotifier::Read, this);
	connect(stdin_notifier, &QSocketNotifier::activated, this, &ShvRshApp::onReadyReadStdIn);

	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
}

ShvRshApp::~ShvRshApp()
{
	shvInfo() << "destroying shv agent application";
	disconnect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::socketConnectedChanged, nullptr, nullptr);
}

ShvRshApp *ShvRshApp::instance()
{
	return qobject_cast<ShvRshApp*>(QCoreApplication::instance());
}

void ShvRshApp::onBrokerConnectedChanged(bool is_connected)
{
	if(is_connected) {
		launchRemoteShell();
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
		//shvInfo() << "RPC request received:" << rq.toPrettyString();
		if(m_shvTree->root()) {
			m_shvTree->root()->handleRpcRequest(rq);
		}
	}
	else if(msg.isResponse()) {
		cp::RpcResponse resp(msg);
		shvMessage() << "RPC response received request id:" << resp.requestId().toCpon() << resp.toPrettyString();
		//shvInfo() << __LINE__ << m_tunnelRequestId << m_tunnelShvPath;
		//shv::chainpack::RpcValue v = rsp.requestId();
		if(resp.requestId() == m_readTunnelRequestId) {
			cp::TunnelCtl tctl = resp.tunnelCtl();
			if(tctl.state() == cp::TunnelCtl::State::CloseTunnel) {
				shvInfo() << "Tunnel closed by peer.";
				quit();
				return;
			}
			if(m_writeTunnelRequestId == 0) {
				if(resp.isError()) {
					shvError() << "Create tunnel error:" << resp.error().toString();
					quit();
					return;
				}
				if(tctl.state() == cp::TunnelCtl::State::CreateTunnelRequest) {
					logTunnelD() << "CreateTunnelRequest received:" << msg.toPrettyString();
					cp::CreateTunnelReqCtl create_tunnel_request(tctl);
					m_writeTunnelRequestId = create_tunnel_request.requestId();
					m_writeTunnelCallerIds = resp.revCallerIds();
					cp::CreateTunnelRespCtl create_tunnel_response;
					cp::RpcMessage resp2;
					resp2.setRequestId(m_writeTunnelRequestId);
					resp2.setCallerIds(m_writeTunnelCallerIds);
					resp2.setTunnelCtl(create_tunnel_response);
					resp2.setRegisterRevCallerIds();
					//resp.setResult(nullptr);
					logTunnelD() << "Sending CreateTunnelResponse:" << resp2.toPrettyString();
					rpcConnection()->sendMessage(resp2);
				}
				else {
					shvError() << "Create tunnel request expected!";
					quit();
				}
				return;
			}
			cp::RpcValue result = resp.result();
			const shv::chainpack::RpcValue::List &lst = result.toList();
			if(lst.empty()) {
				shvError() << "Invalid tunnel message received:" << resp.toCpon();
				quit();
				return;
			}
			int channel = lst.value(0).toInt();
			if(channel == STDOUT_FILENO || channel == STDERR_FILENO) {
				const shv::chainpack::RpcValue::String &blob = lst.value(1).asString();
				writeToOutChannel(channel, blob);
			}
			else {
				shvError() << "Ignoring invalid channel number data received, channel:" << channel;
			}
		}
	}
	else if(msg.isSignal()) {
		cp::RpcSignal ntf(msg);
		shvInfo() << "RPC notify received:" << ntf.toCpon();
	}
}

void ShvRshApp::onReadyReadStdIn()
{
	std::vector<char> data = shv::core::Utils::readAllFd(STDIN_FILENO);
	writeToTunnel(0, cp::RpcValue::String(data.data(), data.size()));
}

void ShvRshApp::writeToTunnel(int channel, const cp::RpcValue &data)
{
	//shvInfo() << m_rpcConnection->isBrokerConnected() << m_tunnelShvPath << "GGG";
	if(!m_rpcConnection->isBrokerConnected() || m_writeTunnelRequestId == 0 || !m_writeTunnelCallerIds.isValid()) {
		shvError() << "Tunnel not open or initiallized, quitting and throwing away data:" << data.toCpon();
		quit();
	}
	else {
		cp::RpcResponse resp;
		resp.setRequestId(m_writeTunnelRequestId);
		resp.setCallerIds(m_writeTunnelCallerIds);
		cp::RpcValue::List result{channel, data};
		resp.setResult(result);
		rpcConnection()->sendMessage(resp);
	}
}

void ShvRshApp::writeToOutChannel(int channel, const std::string &data)
{
	if(channel == STDOUT_FILENO || channel == STDERR_FILENO) {
		std::string &buff = m_channelBuffs[channel];
		buff += data;
		ssize_t n = ::write(channel, buff.data(), buff.size());
		if(n < 0) {
			//shvError() << "Error write remote data to " << (channel == STDOUT_FILENO? "stdout": "stderr") << ::strerror(errno);
			n = 0;
		}
		buff.erase(0, static_cast<size_t>(n));
		if(!buff.empty()) {
			QTimer::singleShot(100, this, [this, channel]() {
				this->writeToOutChannel(channel, std::string());
			});
		}
	}
	else {
		shvError() << "Ignoring invalid channel number, channel:" << channel;
	}
}

void ShvRshApp::launchRemoteShell()
{
	std::string tunp = m_cliOptions->tunnelShvPath();
	std::string tunm = m_cliOptions->tunnelMethod();
	if(!tunp.empty() && !tunm.empty()) {
		struct winsize ws;
		if (::ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) < 0)
			SHV_EXCEPTION("ioctl-TIOCGWINSZ");
		cp::RpcValue::Map params;
		params[cp::Rpc::JSONRPC_METHOD] = "runPtyCmd";
		params[cp::Rpc::JSONRPC_PARAMS] = cp::RpcValue::List{"/bin/bash", ws.ws_col, ws.ws_row};
		m_readTunnelRequestId = m_rpcConnection->callShvMethod(tunp, tunm, params);
		shvDebug() << "opening tunnel on path:" << tunp << "method:" << tunm << "request id:" << m_readTunnelRequestId;
#if 0
		m_rpcConnection->createSubscription(m_tunnelShvPath, cp::Rpc::NTF_CONNECTED_CHANGED);
		{
			cp::RpcRequest rq;
			rq.setShvPath(m_tunnelShvPath);
			rq.setMethod("runPtyCmd");
			rq.setParams(cp::RpcValue::List{"/bin/sh", ws.ws_col, ws.ws_row});
			m_tunnelRequestId = m_rpcConnection->callMethod(rq);
		}
#endif
	}
	else {
		shvError() << "Invalid tunnel path:" << tunp << "method:" << tunm;
		quit();
	}
}

