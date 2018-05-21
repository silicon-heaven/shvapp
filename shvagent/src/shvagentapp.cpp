#include "shvagentapp.h"
#include "appclioptions.h"
#include "sessionprocess.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/tunnelconnection.h>
#include <shv/iotqt/rpc/tunnelhandle.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/node/localfsnode.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/metamethod.h>

#include <shv/core/stringview.h>

#include <QProcess>
#include <QSocketNotifier>
#include <QTimer>
#include <QtGlobal>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif

#ifdef HANDLE_UNIX_SIGNALS
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace cp = shv::chainpack;

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, false},
	{cp::Rpc::METH_CONNECTION_TYPE, cp::MetaMethod::Signature::RetVoid, false},
	{cp::Rpc::METH_RUN_CMD, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LAUNCH_REXEC, cp::MetaMethod::Signature::RetVoid, false},
};

size_t AppRootNode::methodCount()
{
	return meta_methods.size();
}

const shv::chainpack::MetaMethod *AppRootNode::metaMethod(size_t ix)
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
		return ShvAgentApp::instance()->rpcConnection()->connectionType();
	}
	return Super::call(method, params);
}

shv::chainpack::RpcValue AppRootNode::processRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	if(rq.shvPath().toString().empty()) {
		if(rq.method() == cp::Rpc::METH_RUN_CMD) {
			ShvAgentApp *app = ShvAgentApp::instance();
			app->runCmd(rq);
			return cp::RpcValue();
		}
		if(rq.method() == cp::Rpc::METH_LAUNCH_REXEC) {
			ShvAgentApp *app = ShvAgentApp::instance();
			app->launchRexec(rq);
			return cp::RpcValue();
		}
	}
	return Super::processRpcRequest(rq);
}

#ifdef HANDLE_UNIX_SIGNALS
namespace {
int sig_term_socket_ends[2];
QSocketNotifier *sig_term_socket_notifier = nullptr;
//struct sigaction* old_handlers[sizeof(handled_signals) / sizeof(int)];

void signal_handler(int sig, siginfo_t *siginfo, void *context)
{
	Q_UNUSED(siginfo)
	Q_UNUSED(context)
	shvInfo() << "SIG:" << sig;
	char a = sig;
	::write(sig_term_socket_ends[0], &a, sizeof(a));
}

}

void ShvAgentApp::handleUnixSignal()
{
	sig_term_socket_notifier->setEnabled(false);
	char sig;
	::read(sig_term_socket_ends[1], &sig, sizeof(sig));

	shvInfo() << "SIG" << (int)sig << "catched.";
	emit aboutToTerminate((int)sig);

	sig_term_socket_notifier->setEnabled(true);
	shvInfo() << "Terminating application.";
	quit();
}

void ShvAgentApp::installUnixSignalHandlers()
{
	shvInfo() << "installing Unix signals handlers";
	{
		struct sigaction sig_act;
		memset (&sig_act, '\0', sizeof(sig_act));
		// Use the sa_sigaction field because the handles has two additional parameters
		sig_act.sa_sigaction = &signal_handler;
		// The SA_SIGINFO flag tells sigaction() to use the sa_sigaction field, not sa_handler.
		sig_act.sa_flags = SA_SIGINFO;
		const int handled_signals[] = {SIGTERM, SIGINT, SIGHUP, SIGUSR1, SIGUSR2};
		for(int s : handled_signals)
			if	(sigaction(s, &sig_act, 0) > 0)
				shvError() << "Couldn't register handler for signal:" << s;
	}
	if(::socketpair(AF_UNIX, SOCK_STREAM, 0, sig_term_socket_ends))
		qFatal("Couldn't create SIG_TERM socketpair");
	sig_term_socket_notifier = new QSocketNotifier(sig_term_socket_ends[1], QSocketNotifier::Read, this);
	connect(sig_term_socket_notifier, &QSocketNotifier::activated, this, &ShvAgentApp::handleUnixSignal);
	shvInfo() << "SIG_TERM handler installed OK";
}
#endif

ShvAgentApp::ShvAgentApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
#ifdef HANDLE_UNIX_SIGNALS
	installUnixSignalHandlers();
#endif
#ifdef Q_OS_UNIX
	if(0 != ::setpgid(0, 0))
		shvError() << "Error set process group ID:" << errno << ::strerror(errno);
#endif
	cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);

	if(!cli_opts->user_isset())
		cli_opts->setUser("iot");
	if(!cli_opts->password_isset())
		cli_opts->setPassword("lub42DUB");
	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &ShvAgentApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvAgentApp::onRpcMessageReceived);

	AppRootNode *root = new AppRootNode();
	m_shvTree = new shv::iotqt::node::ShvNodeTree(root, this);
	//connect(m_shvTree->root(), &shv::iotqt::node::ShvRootNode::sendRpcMesage, this, &ShvAgentApp::onRootNodeSendRpcMesage);
	m_shvTree->mkdir("sys/rproc");
	QString sys_fs_root_dir = cli_opts->sysFsRootDir();
	if(!sys_fs_root_dir.isEmpty() && QDir(sys_fs_root_dir).exists()) {
		const char *SYS_FS = "sys/fs";
		shvInfo() << "Exporting" << sys_fs_root_dir << "as" << SYS_FS << "node";
		shv::iotqt::node::LocalFSNode *fsn = new shv::iotqt::node::LocalFSNode(sys_fs_root_dir);
		m_shvTree->mount(SYS_FS, fsn);
	}

	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
}

ShvAgentApp::~ShvAgentApp()
{
	shvInfo() << "destroying shv agent application";
}

ShvAgentApp *ShvAgentApp::instance()
{
	return qobject_cast<ShvAgentApp *>(QCoreApplication::instance());
}

void ShvAgentApp::launchRexec(const shv::chainpack::RpcRequest &rq)
{
	using TunnelParams = shv::iotqt::rpc::TunnelParams;
	using TunnelParamsMT = shv::iotqt::rpc::TunnelParams::MetaType;
	TunnelParams tun_params;
	tun_params[TunnelParamsMT::Key::Host] = m_rpcConnection->host();
	tun_params[TunnelParamsMT::Key::Port] = m_rpcConnection->port();
	tun_params[TunnelParamsMT::Key::User] = m_rpcConnection->user();
	tun_params[TunnelParamsMT::Key::Password] = m_rpcConnection->password();
	//tun_params[TunnelParamsMT::Key::ParentClientId] = m_rpcConnection->brokerClientId();
	//tun_params[TunnelParamsMT::Key::RequestId] = rq.requestId();
	//tun_params[TunnelParamsMT::Key::CallerClientIds] = rq.callerIds();
	//tun_params[TunnelParamsMT::Key::TunName] = tun_name;
	//std::string mount_point = ".broker/clients/" + std::to_string(m_rpcConnection->brokerClientId()) + "/rproc/" + name;
	SessionProcess *proc = new SessionProcess(this);
	//proc->setCurrentReadChannel(QProcess::StandardOutput);
	cp::RpcResponse resp1 = cp::RpcResponse::forRequest(rq);
	connect(proc, &SessionProcess::readyReadStandardOutput, [this, proc, resp1]() {
		if(!proc->canReadLine()) {
			return;
		}
		cp::RpcResponse resp = resp1;
		try {
			QByteArray ba = proc->readLine();
			std::string data(ba.constData(), ba.size());
			cp::RpcValue::Map m = cp::RpcValue::fromCpon(data).toMap();
			std::string rel_path = m.value(cp::Rpc::KEY_MOUT_POINT).toString();
			std::string mount_point = m_rpcConnection->brokerMountPoint();
			size_t cnt = shv::core::StringView(mount_point).split('/').size();
			for (size_t i = 0; i < cnt; ++i) {
				rel_path = "../" + rel_path;
			}
			m[cp::Rpc::KEY_MOUT_POINT] = rel_path;
			cp::RpcValue result = m;
			resp.setResult(result);
			shvInfo() << "Got tunnel handle from child process:" << result.toPrettyString();
		}
		catch (std::exception &e) {
			resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodInvocationException, e.what()));
		}
		m_rpcConnection->sendMessage(resp);
	});

	QString app = QCoreApplication::applicationDirPath() + "/shvrexec";
	QStringList params;
	params << "--mtid" << "-v" << "rpcrawmsg";
	shvInfo() << "starting child process:" << app << params.join(' ');
	proc->start(app, params);
	std::string cpon = tun_params.toRpcValue().toCpon();
	proc->write(cpon.data(), cpon.size());
	proc->write("\n", 1);
}

void ShvAgentApp::runCmd(const shv::chainpack::RpcRequest &rq)
{
	SessionProcess *proc = new SessionProcess(this);
	auto rq2 = rq;
	connect(proc, static_cast<void (SessionProcess::*)(int)>(&SessionProcess::finished), [this, proc, rq2](int) {
		QByteArray ba = proc->readAllStandardOutput();
		cp::RpcResponse resp = cp::RpcResponse::forRequest(rq2);
		resp.setResult(std::string(ba.constData(), ba.size()));
		m_rpcConnection->sendMessage(resp);
	});
	proc->start(QString::fromStdString(rq.params().toString()));
}

void ShvAgentApp::onBrokerConnectedChanged(bool is_connected)
{
	Q_UNUSED(is_connected)
	//if(is_connected)
	//	shvInfo() <<
}

void ShvAgentApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		shvInfo() << "RPC request received:" << rq.toPrettyString();
		cp::RpcResponse resp = cp::RpcResponse::forRequest(rq.metaData());
		try {
			//shvInfo() << "RPC request received:" << rq.toCpon();
			const cp::RpcValue shv_path = rq.shvPath();
			std::string path_rest;
			shv::iotqt::node::ShvNode *nd = m_shvTree->cd(shv_path.toString(), &path_rest);
			if(!nd)
				SHV_EXCEPTION("Path not found: " + shv_path.toString());
			rq.setShvPath(path_rest);
			shv::chainpack::RpcValue result = nd->processRpcRequest(rq);
			if(result.isValid())
				resp.setResult(result);
			else
				return;
		}
		catch (shv::core::Exception &e) {
			resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodInvocationException, e.message()));
		}
		if(resp.requestId().toInt() > 0) // RPC calls with requestID == 0 does not expect response
			m_rpcConnection->sendMessage(resp);
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rp(msg);
		shvInfo() << "RPC response received:" << rp.toPrettyString();
	}
	else if(msg.isNotify()) {
		cp::RpcNotify nt(msg);
		shvInfo() << "RPC notify received:" << nt.toPrettyString();
		/*
		if(nt.method() == cp::Rpc::NTF_VAL_CHANGED) {
			if(nt.shvPath() == "/test/shv/lublicator2/status") {
				shvInfo() << lublicatorStatusToString(nt.params().toUInt());
			}
		}
		*/
	}
}
/*
void ShvAgentApp::onRootNodeSendRpcMesage(const shv::chainpack::RpcMessage &msg)
{
	if(msg.isResponse()) {
		cp::RpcResponse resp(msg);
		if(resp.requestId().toUInt() == 0) // RPC calls with requestID == 0 does not expect response
			return;
		m_rpcConnection->sendMessage(resp);
		return;
	}
	shvError() << "Send message not implemented.";// << msg.toCpon();
}
*/
