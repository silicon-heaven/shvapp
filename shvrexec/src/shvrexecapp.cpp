#include "shvrexecapp.h"
#include "appclioptions.h"
#include "childprocess.h"

#include <shv/iotqt/rpc/tunnelconnection.h>
#include <shv/iotqt/node/shvnodetree.h>

#include <shv/coreqt/log.h>
#include <shv/chainpack/metamethod.h>

#include <QProcess>
#include <QTimer>

#include <iostream>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif

namespace cp = shv::chainpack;

const char METH_WRITE[] = "write";

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, false},
	{cp::Rpc::METH_CONNECTION_TYPE, cp::MetaMethod::Signature::RetVoid, false},
	//{cp::Rpc::KEY_TUNNEL_HANDLE, cp::MetaMethod::Signature::RetVoid, false},
	{METH_WRITE, cp::MetaMethod::Signature::RetParam, false},
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
	if(method == METH_WRITE) {
		const shv::chainpack::RpcValue::Blob data = params.toBlob();
		qint64 len = ShvRExecApp::instance()->writeProcessStdin(data.data(), data.size());
		if(len < 0)
			shvError() << "Error writing process stdin.";
		return cp::RpcValue{};
	}
	return Super::call(method, params);
}

/*
shv::chainpack::RpcValue AppRootNode::dir(const std::string &shv_path, const shv::chainpack::RpcValue &methods_params)
{
	Q_UNUSED(shv_path)
	cp::RpcValue::List ret = Super::dir(methods_params).toList();
	ret.push_back(cp::Rpc::METH_APP_NAME);
	ret.push_back(cp::Rpc::METH_CONNECTION_TYPE);
	ret.push_back(cp::Rpc::KEY_TUNNEL_HANDLE);
	ret.push_back(METH_STDIN_WRITE);
	return ret;
}

shv::chainpack::RpcValue AppRootNode::call(const std::string &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(method == cp::Rpc::METH_APP_NAME) {
		return QCoreApplication::instance()->applicationName().toStdString();
	}
	if(method == cp::Rpc::METH_CONNECTION_TYPE) {
		return ShvRExecApp::instance()->rpcConnection()->connectionType();
	}
	if(method == cp::Rpc::KEY_TUNNEL_HANDLE) {
		return ShvRExecApp::instance()->rpcConnection()->tunnelHandle();
	}
	if(method == METH_STDIN_WRITE) {
		const shv::chainpack::RpcValue::String &data = params.toString();
		qint64 len = ShvRExecApp::instance()->writeProcessStdin(data.data(), data.size());
		if(len < 0)
			shvError() << "Error writing process stdin.";
		return cp::RpcValue{};
	}
	return Super::call(shv_path, method, params);
}
*/
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

void ShvRExecApp::onBrokerConnectedChanged(bool is_connected)
{
	if(is_connected) {
		QString exec_cmd = m_cliOptions->execCommand();
		shvInfo() << "Starting process:" << exec_cmd;
		QStringList sl = exec_cmd.split(' ', QString::SkipEmptyParts);
		QString program = sl.value(0);
		QStringList arguments = sl.mid(1);
		m_cmdProc = new ChildProcess(this);
		connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, m_cmdProc, &QProcess::terminate);
		connect(m_cmdProc, &QProcess::readyReadStandardOutput, this, &ShvRExecApp::onReadyReadProcessStandardOutput);
		connect(m_cmdProc, &QProcess::readyReadStandardError, this, &ShvRExecApp::onReadyReadProcessStandardError);
		connect(m_cmdProc, &QProcess::errorOccurred, [](QProcess::ProcessError error) {
			shvError() << "Exec process error:" << error;
			quit();
		});
		connect(m_cmdProc, &QProcess::stateChanged, [this](QProcess::ProcessState state) {
			shvInfo() << "Exec process new state:" << state;
			if(state == QProcess::Running) {
				/// send tunnel handle to agent
				cp::RpcValue::Map ret;
				unsigned cli_id = m_rpcConnection->brokerClientId();
				ret["clientPath"] = std::string(cp::Rpc::DIR_BROKER) + "/" + cp::Rpc::DIR_CLIENTS + "/" + std::to_string(cli_id);
				std::string s = cp::RpcValue(ret).toCpon();
				shvInfo() << "Process" << m_cmdProc->program() << "started, stdout:" << s;
				std::cout << s << "\n";
				std::cout.flush(); /// necessary sending '\n' is not enough to flush stdout
			}
		});
		connect(m_cmdProc, QOverload<int>::of(&QProcess::finished), this, [this](int exit_code) {
			shvInfo() << "Process" << m_cmdProc->program() << "finished with exit code:" << exit_code;
			quit();
		});
		m_cmdProc->start(program, arguments);
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

qint64 ShvRExecApp::writeProcessStdin(const char *data, size_t len)
{
	if(!m_cmdProc) {
		shvError() << "Attempt to write to not existing process.";
		return -1;
	}
	qint64 n = m_cmdProc->write(data, len);
	if(n != (qint64)len) {
		shvError() << "Write process stdin error, only" << n << "of" << len << "bytes written.";
	}
	return n;
}

void ShvRExecApp::onReadyReadProcessStandardOutput()
{
	//m_cmdProc->setReadChannel(QProcess::StandardOutput);
	QByteArray ba = m_cmdProc->readAllStandardOutput();
	sendProcessOutput(1, ba);
}

void ShvRExecApp::onReadyReadProcessStandardError()
{
	QByteArray ba = m_cmdProc->readAllStandardError();
	sendProcessOutput(2, ba);
}

void ShvRExecApp::sendProcessOutput(int channel, const QByteArray &data)
{
	if(!m_rpcConnection->isBrokerConnected()) {
		shvError() << "Broker is not connected, throwing away process channel:" << channel << "data:" << data;
		quit();
	}
	else {
		const shv::iotqt::rpc::TunnelParams &pars = m_rpcConnection->tunnelParams();
		cp::RpcResponse resp;
		resp.setRequestId(pars.value(shv::iotqt::rpc::TunnelParams::MetaType::Key::RequestId));
		resp.setCallerIds(pars.value(shv::iotqt::rpc::TunnelParams::MetaType::Key::CallerClientIds));
		cp::RpcValue::List result;
		result.push_back(channel);
		result.push_back(cp::RpcValue::Blob(data.constData(), data.size()));
		resp.setResult(result);
		m_rpcConnection->sendMessage(resp);
	}
}

