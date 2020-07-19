#include "shvrexecapp.h"
#include "appclioptions.h"
#include "ptyprocess.h"
#include "process.h"

#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/node/shvnodetree.h>

#include <shv/coreqt/log.h>
#include <shv/core/stringview.h>
#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/tunnelctl.h>

#include <QProcess>
#include <QTimer>

#include <iostream>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif

namespace cp = shv::chainpack;
namespace si = shv::iotqt;

//const char METH_SETWINSZ[] = "setWinSize";
const char METH_RUNCMD[] = "runCmd";
const char METH_RUNPTYCMD[] = "runPtyCmd";
//const char METH_WRITE[] = "write";

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, false},
	//{cp::Rpc::METH_CONNECTION_TYPE, cp::MetaMethod::Signature::RetVoid, false},
	//{cp::Rpc::KEY_TUNNEL_HANDLE, cp::MetaMethod::Signature::RetVoid, false},
	//{METH_SETWINSZ, cp::MetaMethod::Signature::RetParam, false},
	//{METH_RUNCMD, cp::MetaMethod::Signature::RetParam, false},
	//{METH_RUNPTYCMD, cp::MetaMethod::Signature::RetParam, false},
	//{METH_WRITE, cp::MetaMethod::Signature::RetParam, false},
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

shv::chainpack::RpcValue AppRootNode::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == cp::Rpc::METH_APP_NAME) {
			return QCoreApplication::instance()->applicationName().toStdString();
		}
		//if(method == cp::Rpc::METH_CONNECTION_TYPE) {
		//	return ShvRExecApp::instance()->rpcConnection()->connectionType();
		//}
		/*
		if(method == METH_SETWINSZ) {
			const shv::chainpack::RpcValue::List &list = params.toList();
			ShvRExecApp::instance()->setTerminalWindowSize(list.value(0).toInt(), list.value(1).toInt());
			return true;
		}
		*/
	}
	return Super::callMethod(shv_path, method, params);
}

ShvRExecApp::ShvRExecApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
//#ifdef Q_OS_UNIX
//	if(0 != ::setpgid(0, 0))
//		shvError() << "Error set process group ID:" << errno << ::strerror(errno);
//#endif
	//cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	m_rpcConnection = new shv::iotqt::rpc::ClientConnection(this);

	cp::RpcValue startup_opts;
	{
		std::string s;
		std::getline(std::cin, s);
		shvInfo() << "startup:" << s;
		startup_opts = cp::RpcValue::fromCpon(s);
	}

	cli_opts->setHeartbeatInterval(0);
	cli_opts->setReconnectInterval(0);
	m_rpcConnection->setCliOptions(cli_opts);
	{
		//using ConnectionParams = shv::iotqt::rpc::ConnectionParams;
		//using ConnectionParamsMT = shv::iotqt::rpc::ConnectionParams::MetaType;
		//const TunnelParams &m = m_rpcConnection->tunnelParams();
		cp::FindTunnelRespCtl find_tunnel_response(startup_opts.at("findTunnelResponse"));
		m_tunnelCtl = find_tunnel_response;
		m_rpcConnection->setHost(find_tunnel_response.host());
		m_rpcConnection->setPort(find_tunnel_response.port());
		shv::chainpack::RpcValue::Map tunnel_opts;
		tunnel_opts[cp::Rpc::KEY_SECRET] = find_tunnel_response.secret();
		m_rpcConnection->setTunnelOptions(tunnel_opts);
		m_onConnectedCall = startup_opts.at("onConnectedCall");
	}

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::socketConnectedChanged, [](bool connected) {
		if(!connected) {
			shvError() << "Socket disconnected, quitting";
			quit();
		}
	});
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &ShvRExecApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvRExecApp::onRpcMessageReceived);

	AppRootNode *root = new AppRootNode();
	m_shvTree = new shv::iotqt::node::ShvNodeTree(root, this);
	connect(m_shvTree->root(), &shv::iotqt::node::ShvRootNode::sendRpcMessage, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::sendMessage);

	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);

	m_inactivityTimer = new QTimer(this);
	m_inactivityTimer->setSingleShot(true);
	static constexpr int TO = 1000 * 60 * 60 * 12;
	//static constexpr int TO = 1000 * 30;
	m_inactivityTimer->setInterval(TO);
	connect(m_inactivityTimer, &QTimer::timeout, [this]() {
		shvError() << "Inactivity timer expired after" << (m_inactivityTimer->interval()/1000) << "sec.";
		this->closeAndQuit();
	});
	m_inactivityTimer->start();
}

ShvRExecApp::~ShvRExecApp()
{
	shvInfo() << "destroying shv agent application";
	disconnect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::socketConnectedChanged, 0, 0);
}

ShvRExecApp *ShvRExecApp::instance()
{
	return qobject_cast<ShvRExecApp*>(QCoreApplication::instance());
}

/*
void ShvRExecApp::setTerminalWindowSize(int w, int h)
{
	m_termWidth = w;
	m_termHeight = h;
}
*/
void ShvRExecApp::onBrokerConnectedChanged(bool is_connected)
{
	if(is_connected) {
		if(m_tunnelCtl.state() == cp::TunnelCtl::State::FindTunnelResponse) {
			cp::FindTunnelRespCtl find_tunnel_response = cp::FindTunnelRespCtl(m_tunnelCtl);
			cp::CreateTunnelReqCtl create_tunnel_request = cp::CreateTunnelReqCtl::fromFindTunnelResponse(find_tunnel_response);
			// generate ID for messages read from tunnel, which is different
			// than request ID used to open tunnel
			// this original request ID is used for writing to the tunnel
			create_tunnel_request.setRequestId(rpcConnection()->nextRequestId());
			m_tunnelCtl = create_tunnel_request;
			m_writeTunnelRequestId = find_tunnel_response.requestId();
			m_readTunnelRequestId = create_tunnel_request.requestId();
			shv::chainpack::RpcResponse resp;
			resp.setRequestId(m_writeTunnelRequestId);
			resp.setCallerIds(find_tunnel_response.callerIds());
			resp.setTunnelCtl(m_tunnelCtl);
			resp.setRegisterRevCallerIds();
			rpcConnection()->sendMessage(resp);

			si::rpc::RpcResponseCallBack *cb = new si::rpc::RpcResponseCallBack(m_readTunnelRequestId, this);
			connect(rpcConnection(), &si::rpc::ClientConnection::rpcMessageReceived, cb, &si::rpc::RpcResponseCallBack::onRpcMessageReceived);
			cb->start([this](const shv::chainpack::RpcResponse &resp) {
				cp::TunnelCtl tctl = resp.tunnelCtl();
				if(tctl.state() == cp::TunnelCtl::State::CreateTunnelResponse) {
					cp::CreateTunnelRespCtl create_tunnel_response(tctl);
					m_writeTunnelCallerIds = resp.revCallerIds();
					const shv::chainpack::RpcValue::Map &call_p = m_onConnectedCall.toMap();
					const shv::chainpack::RpcValue::String &method = call_p.value(cp::Rpc::JSONRPC_METHOD).toString();
					const shv::chainpack::RpcValue &params = call_p.value(cp::Rpc::JSONRPC_PARAMS);
					/*
					int rqid = call_p.value(cp::Rpc::JSONRPC_REQUEST_ID).toInt();
					shv::chainpack::RpcValue cids = call_p.value(cp::Rpc::JSONRPC_CALLER_ID);
					cp::RpcRequest rq;
					rq.setMethod(method);
					rq.setParams(params);
					rq.setRequestId(rqid);
					rq.setCallerIds(cids);
					onRpcMessageReceived(rq);
					*/
					if(method == METH_RUNPTYCMD) {
						runPtyCmd(params);
						return;
					}
					else if(method == METH_RUNCMD) {
						runCmd(params);
						return;
					}
					shvError() << "Invalid shvrexec method:" << method;
				}
				else if(tctl.state() == cp::TunnelCtl::State::CloseTunnel) {
					shvInfo() << "Tunnel closed";
				}
				else {
					shvError() << "Invalid response to FindTunnelRequest:" << resp.toPrettyString();
				}
				quit();
			});
			return;
		}
		else {
			shvError() << "Invalid tunnel state";
		}
	}
	else {
		/// once connection to tunnel is lost, tunnel handle becomes invalid, destroy tunnel
	}
	quit();
}

void ShvRExecApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	m_inactivityTimer->start();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		m_shvTree->root()->handleRpcRequest(rq);
		/*
		//shvInfo() << "RPC request received:" << rq.toCpon();
		const cp::RpcValue shv_path = rq.shvPath();
		if(rq.shvPath().toString().empty()) {
			if(rq.method() == METH_RUNPTYCMD) {
				runPtyCmd(rq.params());
				return;
			}
			else if(rq.method() == METH_RUNCMD) {
				runCmd(rq.params());
				return;
			}
		}
		else {
			m_shvTree->root()->handleRpcRequest(rq);
		}
		*/
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rp(msg);
		shvInfo() << "RPC response received:" << rp.toCpon();
		if(rp.requestId() == m_readTunnelRequestId) {
			shv::chainpack::RpcValue result = rp.result();
			int channel = 0;
			shv::chainpack::RpcValue data;
			if(result.isList()) {
				const shv::chainpack::RpcValue::List &lst = result.toList();
				channel = lst.value(0).toInt();
				data = lst.value(1);
			}
			else {
				data = result;
			}
			if(channel == 0) {
				const shv::chainpack::RpcValue::String &blob = data.toString();
				if(blob.size()) {
					qint64 len = ShvRExecApp::instance()->writeCmdProcessStdIn(blob.data(), blob.size());
					if(len < 0)
						shvError() << "Error writing process stdin.";
				}
				else {
					shvError() << "Invalid data received:" << result.toCpon();
				}
			}
		}
	}
	else if(msg.isSignal()) {
		cp::RpcSignal ntf(msg);
		/*
		const cp::RpcValue shv_path = ntf.shvPath();
		if(shv_path.toString() == "in") {
			const shv::chainpack::RpcValue::Blob &data = ntf.params().toBlob();
			writeProcessStdin(data.data(), data.size());
			return;
		}
		*/
		shvInfo() << "RPC notify received:" << ntf.toCpon();
	}
}
/*
void ShvRExecApp::openTunnel(const std::string &method, const shv::chainpack::RpcValue &params, int request_id, const shv::chainpack::RpcValue &cids)
{
	cp::RpcResponse resp;
	try {
		resp.setCallerIds(cids);
		resp.setRequestId(request_id);
		if(method == METH_RUNPTYCMD) {
			runPtyCmd(params);
		}
		else if(method == METH_RUNCMD) {
			runCmd(params);
		}
		else {
			SHV_EXCEPTION("Invalid method name: " + method);
		}
		m_writeTunnelHandle = shv::iotqt::rpc::TunnelHandle(request_id, cids);
		m_readTunnelRequestId = m_rpcConnection->nextRequestId();
		resp.setResult(m_readTunnelRequestId);
		resp.setRegisterRevCallerIds();
	}
	catch (shv::core::Exception &e) {
		resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, e.message()));
	}
	m_rpcConnection->sendMessage(resp);
}
*/
void ShvRExecApp::runCmd(const shv::chainpack::RpcValue &params)
{
	if(m_cmdProc || m_ptyCmdProc)
		SHV_EXCEPTION("Process running already");

	shvInfo() << "Starting process:" << params.toCpon();
	m_cmdProc = new Process(this);
	connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, m_cmdProc, &QProcess::kill);
	connect(m_cmdProc, &QProcess::readyReadStandardOutput, this, &ShvRExecApp::onReadyReadProcessStandardOutput);
	connect(m_cmdProc, &QProcess::readyReadStandardError, this, &ShvRExecApp::onReadyReadProcessStandardError);
	connect(m_cmdProc, &QProcess::errorOccurred, [](QProcess::ProcessError error) {
		shvError() << "Exec process error:" << error;
		//this->closeAndQuit();
	});
	connect(m_cmdProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this](int exit_code, QProcess::ExitStatus status) {
		switch (status) {
		case QProcess::NormalExit:
			shvInfo() << "Process" << m_cmdProc->program() << "finished with exit code:" << exit_code;
			break;
		case QProcess::CrashExit:
			shvWarning() << "Process" << m_cmdProc->program() << "crashed with exit code:" << exit_code;
			break;
		}
		this->closeAndQuit();
	});
	const shv::chainpack::RpcValue::List &cmd_lst = params.toList();
	QString cmd = QString::fromStdString(params.isString()? params.toString(): cmd_lst.value(0).toString());
	QStringList pars;
	for (size_t i = 1; i < cmd_lst.size(); ++i)
		pars << QString::fromStdString(cmd_lst[i].toString());
	m_cmdProc->start(cmd, pars);
}

void ShvRExecApp::runPtyCmd(const shv::chainpack::RpcValue &params)
{
	if(m_cmdProc || m_ptyCmdProc)
		SHV_EXCEPTION("Process running already");

	const shv::chainpack::RpcValue::List lst = params.toList();
	shv::chainpack::RpcValue cmd_params = lst.value(0);
	if(!cmd_params.isString() && !cmd_params.isList()) {
		shvError() << "Invalid command:" << cmd_params.toCpon();
		closeAndQuit();
	}
	int pty_cols = lst.value(1).toInt();
	int pty_rows = lst.value(2).toInt();

	shvInfo() << "Starting process:" << cmd_params.toCpon();
	m_ptyCmdProc = new PtyProcess(this);
	connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, m_ptyCmdProc, &QProcess::kill);
	connect(m_ptyCmdProc, &PtyProcess::readyReadMasterPty, this, &ShvRExecApp::onReadyReadMasterPty);
	connect(m_ptyCmdProc, &QProcess::errorOccurred, [](QProcess::ProcessError error) {
		shvError() << "Exec process error:" << error;
		//closeAndQuit();
	});
	/*
	connect(m_ptyCmdProc, &QProcess::stateChanged, [this](QProcess::ProcessState state) {
		shvInfo() << "Exec process new state:" << state;
		if(state == QProcess::Running) {
			/// send tunnel handle to agent
			cp::RpcValue::Map ret;
			unsigned cli_id = m_rpcConnection->brokerClientId();
			ret["clientPath"] = std::string(cp::Rpc::DIR_BROKER) + "/" + cp::Rpc::DIR_CLIENTS + "/" + std::to_string(cli_id);
			std::string s = cp::RpcValue(ret).toCpon();
			shvInfo() << "Process" << m_ptyCmdProc->program() << "started, stdout:" << s;
			std::cout << s << "\n";
			std::cout.flush(); /// necessary sending '\n' is not enough to flush stdout
		}
	});
	*/
	connect(m_ptyCmdProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this](int exit_code) {
		shvInfo() << "Process" << m_ptyCmdProc->program() << "finished with exit code:" << exit_code;
		m_ptyCmdProc->disconnect();
		closeAndQuit();
	});
	QString cmd = QString::fromStdString(cmd_params.isString()? cmd_params.toString(): cmd_params.toList().value(0).toString());
	QStringList pars;
	const shv::chainpack::RpcValue::List &cmd_lst = cmd_params.toList();
	for (size_t i = 1; i < cmd_lst.size(); ++i)
		pars << QString::fromStdString(cmd_lst[i].toString());
	m_ptyCmdProc->ptyStart(cmd, pars, pty_cols, pty_rows);
}

qint64 ShvRExecApp::writeCmdProcessStdIn(const char *data, size_t len)
{
	if(m_ptyCmdProc) {
		qint64 n = m_ptyCmdProc->writePtyMaster(data, len);
		if(n != (qint64)len) {
			shvError() << "Write PTY master stdin error, only" << n << "of" << len << "bytes written.";
		}
		return n;
	}
	if(m_cmdProc) {
		qint64 n = m_cmdProc->write(data, len);
		if(n != (qint64)len) {
			shvError() << "Write process stdin error, only" << n << "of" << len << "bytes written.";
		}
		return n;
	}
	shvError() << "Attempt to write to not existing process.";
	return -1;
}

void ShvRExecApp::onReadyReadProcessStandardOutput()
{
	//m_cmdProc->setReadChannel(QProcess::StandardOutput);
	QByteArray ba = m_cmdProc->readAllStandardOutput();
	sendProcessOutput(1, ba.constData(), ba.size());
}

void ShvRExecApp::onReadyReadProcessStandardError()
{
	QByteArray ba = m_cmdProc->readAllStandardError();
	sendProcessOutput(2, ba.constData(), ba.size());
}

void ShvRExecApp::onReadyReadMasterPty()
{
	std::vector<char> data = m_ptyCmdProc->readAllMasterPty();
	sendProcessOutput(1, data.data(), data.size());
}

void ShvRExecApp::sendProcessOutput(int channel, const char *data, size_t data_len)
{
	if(!m_rpcConnection->isBrokerConnected()) {
		shvError() << "Broker is not connected, throwing away process channel:" << channel << "data:" << data;
		quit();
	}
	else {
		m_inactivityTimer->start();
		//const shv::iotqt::rpc::TunnelParams &pars = m_rpcConnection->tunnelParams();
		cp::RpcResponse resp;
		resp.setRequestId(m_writeTunnelRequestId);
		resp.setCallerIds(m_writeTunnelCallerIds);
		cp::RpcValue::List result;
		result.push_back(channel);
		result.push_back(cp::RpcValue::String(data, data_len));
		resp.setResult(result);
		shvDebug() << "sending child process output:" << resp.toPrettyString();
		m_rpcConnection->sendMessage(resp);
	}
}

void ShvRExecApp::closeAndQuit()
{
	//shvInfo() << "closeAndQuit()";
	if(m_rpcConnection->isBrokerConnected() && m_writeTunnelRequestId > 0 && m_writeTunnelCallerIds.isValid()) {
		cp::RpcResponse resp;
		resp.setRequestId(m_writeTunnelRequestId);
		resp.setCallerIds(m_writeTunnelCallerIds);
		cp::RpcValue result = nullptr;
		resp.setResult(result);
		resp.setTunnelCtl(cp::TunnelCtl(cp::TunnelCtl::State::CloseTunnel));
		shvInfo() << "closing tunnel:" << resp.toPrettyString();
		m_rpcConnection->sendMessage(resp);
	}
	QTimer::singleShot(10, this, &ShvRExecApp::quit);
}

