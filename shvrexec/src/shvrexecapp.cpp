#include "shvrexecapp.h"
#include "appclioptions.h"
#include "ptyprocess.h"

#include <shv/iotqt/rpc/tunnelconnection.h>
#include <shv/iotqt/rpc/tunnelhandle.h>
#include <shv/iotqt/node/shvnodetree.h>

#include <shv/coreqt/log.h>
#include <shv/chainpack/metamethod.h>
#include <shv/core/stringview.h>

#include <QProcess>
#include <QTimer>

#include <iostream>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif

namespace cp = shv::chainpack;

//const char METH_SETWINSZ[] = "setWinSize";
const char METH_RUNCMD[] = "runCmd";
const char METH_RUNPTYCMD[] = "runPtyCmd";
//const char METH_WRITE[] = "write";

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, false},
	{cp::Rpc::METH_CONNECTION_TYPE, cp::MetaMethod::Signature::RetVoid, false},
	//{cp::Rpc::KEY_TUNNEL_HANDLE, cp::MetaMethod::Signature::RetVoid, false},
	//{METH_SETWINSZ, cp::MetaMethod::Signature::RetParam, false},
	{METH_RUNCMD, cp::MetaMethod::Signature::RetParam, false},
	{METH_RUNPTYCMD, cp::MetaMethod::Signature::RetParam, false},
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
		return ShvRExecApp::instance()->rpcConnection()->connectionType();
	}
	//if(method == cp::Rpc::KEY_TUNNEL_HANDLE) {
	//	return ShvRExecApp::instance()->rpcConnection()->tunnelHandle();
	//}
	/*
	if(method == METH_SETWINSZ) {
		const shv::chainpack::RpcValue::List &list = params.toList();
		ShvRExecApp::instance()->setTerminalWindowSize(list.value(0).toInt(), list.value(1).toInt());
		return true;
	}
	*/
	/*
	if(method == METH_WRITE) {
		int channel = 0;
		shv::chainpack::RpcValue data;
		if(params.isList()) {
			const shv::chainpack::RpcValue::List &lst = params.toList();
			channel = lst.value(0).toInt();
			data = lst.value(1);
		}
		else {
			data = params;
		}
		if(channel == 0) {
			const shv::chainpack::RpcValue::Blob &blob = data.toBlob();
			if(blob.size()) {
				qint64 len = ShvRExecApp::instance()->writeCmdProcessStdIn(blob.data(), blob.size());
				if(len < 0)
					shvError() << "Error writing process stdin.";
			}
			else {
				shvError() << "Invalid data received:" << params.toCpon();
			}
		}
		return cp::RpcValue{};
	}
	*/
	return Super::call(method, params);
}

shv::chainpack::RpcValue AppRootNode::processRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	if(rq.shvPath().toString().empty()) {
		if(rq.method() == METH_RUNCMD) {
			return ShvRExecApp::instance()->runCmd(rq);
		}
		if(rq.method() == METH_RUNPTYCMD) {
			return ShvRExecApp::instance()->runPtyCmd(rq);
		}
	}
	return Super::processRpcRequest(rq);
}

ShvRExecApp::ShvRExecApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
#ifdef Q_OS_UNIX
	if(0 != ::setpgid(0, 0))
		shvError() << "Error set process group ID:" << errno << ::strerror(errno);
#endif
	cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	m_rpcConnection = new shv::iotqt::rpc::TunnelConnection(this);
	{
		std::string tunnel_params;
		std::getline(std::cin, tunnel_params);
		cp::RpcValue v = cp::RpcValue::fromCpon(tunnel_params);
		m_rpcConnection->setTunnelParams(v.toIMap());
	}

	cli_opts->setHeartbeatInterval(0);
	cli_opts->setReconnectInterval(0);
	m_rpcConnection->setCliOptions(cli_opts);
	{
		using TunnelParams = shv::iotqt::rpc::TunnelParams;
		using TunnelParamsMT = shv::iotqt::rpc::TunnelParams::MetaType;
		const TunnelParams &m = m_rpcConnection->tunnelParams();
		m_rpcConnection->setHost(m.value(TunnelParamsMT::Key::Host).toString());
		m_rpcConnection->setPort(m.value(TunnelParamsMT::Key::Port).toInt());
		m_rpcConnection->setUser(m.value(TunnelParamsMT::Key::User).toString());
		m_rpcConnection->setPassword(m.value(TunnelParamsMT::Key::Password).toString());
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
	/*
	m_shvTree->mkdir("sys");
	QString sys_fs_root_dir = cli_opts->sysFsRootDir();
	if(!sys_fs_root_dir.isEmpty() && QDir(sys_fs_root_dir).exists()) {
		const char *SYS_FS = "sys/fs";
		shvInfo() << "Exporting" << sys_fs_root_dir << "as" << SYS_FS << "node";
		shv::iotqt::node::LocalFSNode *fsn = new shv::iotqt::node::LocalFSNode(sys_fs_root_dir);
		m_shvTree->mount(SYS_FS, fsn);
	}
	*/

	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
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
		/// send tunnel handle to agent
		cp::RpcValue::Map ret;
		//unsigned cli_id = m_rpcConnection->brokerClientId();
		ret[cp::Rpc::KEY_MOUT_POINT] = m_rpcConnection->brokerMountPoint();
		std::string s = cp::RpcValue(ret).toCpon();
		//shvInfo() << "Process" << m_cmdProc->program() << "started, stdout:" << s;
		std::cout << s << "\n";
		std::cout.flush(); /// necessary sending '\n' is not enough to flush stdout
		/*
		QString exec_cmd = m_cliOptions->execCommand();
		if(!exec_cmd.isEmpty()) {
			if(m_cliOptions->winSize_isset()) {
				QString ws = m_cliOptions->winSize();
				QStringList sl = ws.split('x');
				setTerminalWindowSize(sl.value(0).toInt(), sl.value(1).toInt());
			}
			runPtyCmd(exec_cmd.toStdString());
		}
		*/
	}
	else {
		/// once connection to tunnel is lost, tunnel handle becomes invalid, destroy tunnel
		quit();
	}
}

void ShvRExecApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
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
				const shv::chainpack::RpcValue::Blob &blob = data.toBlob();
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
	else if(msg.isNotify()) {
		cp::RpcNotify ntf(msg);
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

cp::RpcValue ShvRExecApp::runCmd(const shv::chainpack::RpcRequest &rq)
{
	if(m_cmdProc || m_ptyCmdProc)
		SHV_EXCEPTION("Process running already");

	shv::chainpack::RpcValue rev_caller_ids = rq.revCallerIds();
	if(!rev_caller_ids.isValid())
		SHV_EXCEPTION("Cannot open tunnel without request with OpenTunnelFlag set!");

	const shv::chainpack::RpcValue::String exec_cmd = rq.params().toString();

	shvInfo() << "Starting process:" << exec_cmd;
	m_cmdProc = new QProcess(this);
	connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, m_cmdProc, &QProcess::terminate);
	connect(m_cmdProc, &QProcess::readyReadStandardOutput, this, &ShvRExecApp::onReadyReadProcessStandardOutput);
	connect(m_cmdProc, &QProcess::readyReadStandardError, this, &ShvRExecApp::onReadyReadProcessStandardError);
	connect(m_cmdProc, &QProcess::errorOccurred, [](QProcess::ProcessError error) {
		shvError() << "Exec process error:" << error;
		//this->closeAndQuit();
	});
	connect(m_cmdProc, QOverload<int>::of(&QProcess::finished), this, [this](int exit_code) {
		shvInfo() << "Process" << m_cmdProc->program() << "finished with exit code:" << exit_code;
		this->closeAndQuit();
	});
	m_cmdProc->start(QString::fromStdString(exec_cmd));
	m_writeTunnelHandle = shv::iotqt::rpc::TunnelHandle(rq.requestId(), rq.callerIds());
	m_readTunnelRequestId = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::TunnelHandle th(m_readTunnelRequestId, rev_caller_ids);
	return th.toRpcValue();
}

shv::chainpack::RpcValue ShvRExecApp::runPtyCmd(const shv::chainpack::RpcRequest &rq)
{
	if(m_cmdProc || m_ptyCmdProc)
		SHV_EXCEPTION("Process running already");

	shv::chainpack::RpcValue rev_caller_ids = rq.revCallerIds();
	if(!rev_caller_ids.isValid())
		SHV_EXCEPTION("Cannot open tunnel without request with OpenTunnelFlag set!");

	const shv::chainpack::RpcValue::List lst = rq.params().toList();
	std::string exec_cmd = lst.value(0).toString();
	int pty_cols = lst.value(1).toInt();
	int pty_rows = lst.value(2).toInt();

	shvInfo() << "Starting process:" << exec_cmd;
	m_ptyCmdProc = new PtyProcess(this);
	connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, m_ptyCmdProc, &QProcess::terminate);
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
	connect(m_ptyCmdProc, QOverload<int>::of(&QProcess::finished), this, [this](int exit_code) {
		shvInfo() << "Process" << m_ptyCmdProc->program() << "finished with exit code:" << exit_code;
		closeAndQuit();
	});
	m_ptyCmdProc->ptyStart(exec_cmd, pty_cols, pty_rows);
	m_writeTunnelHandle = shv::iotqt::rpc::TunnelHandle(rq.requestId(), rq.callerIds());
	m_readTunnelRequestId = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::TunnelHandle th(m_readTunnelRequestId, rev_caller_ids);
	return th.toRpcValue();
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
		//const shv::iotqt::rpc::TunnelParams &pars = m_rpcConnection->tunnelParams();
		cp::RpcResponse resp;
		resp.setRequestId(m_writeTunnelHandle.requestId());
		resp.setCallerIds(m_writeTunnelHandle.callerIds());
		cp::RpcValue::List result;
		result.push_back(channel);
		result.push_back(cp::RpcValue::Blob(data, data_len));
		resp.setResult(result);
		shvInfo() << "sending child process output:" << resp.toPrettyString();
		m_rpcConnection->sendMessage(resp);
	}
}

void ShvRExecApp::closeAndQuit()
{
	/// too late
	//const char TERMINATED[] = "terminated";
	//sendProcessOutput(STDERR_FILENO+1, TERMINATED, sizeof(TERMINATED));
	quit();
}

