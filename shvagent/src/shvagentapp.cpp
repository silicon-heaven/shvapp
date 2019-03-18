#include "shvagentapp.h"
#include "appclioptions.h"
#include "sessionprocess.h"

#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/node/localfsnode.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/tunnelctl.h>
#include <shv/chainpack/metamethod.h>

#include <shv/core/stringview.h>

#include <QProcess>
#include <QSocketNotifier>
#include <QTimer>
#include <QtGlobal>
#include <QFileInfo>
#include <QTemporaryFile>

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
namespace si = shv::iotqt;

static const char M_RUN_SCRIPT[] = "runScript";

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_BROWSE},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::GRANT_BROWSE},
	//{cp::Rpc::METH_CONNECTION_TYPE, cp::MetaMethod::Signature::RetVoid, 0, cp::Rpc::GRANT_BROWSE},
	{cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::GRANT_BROWSE},
	{cp::Rpc::METH_HELP, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_BROWSE},
	{cp::Rpc::METH_RUN_CMD, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_COMMAND},
	{M_RUN_SCRIPT, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_COMMAND},
	{cp::Rpc::METH_LAUNCH_REXEC, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_COMMAND},
};

size_t AppRootNode::methodCount(const StringViewList &shv_path)
{
	if(shv_path.empty())
		return meta_methods.size();
	return 0;
}

const shv::chainpack::MetaMethod *AppRootNode::metaMethod(const StringViewList &shv_path, size_t ix)
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
		if(method == cp::Rpc::METH_HELP) {
			std::string meth = params.toString();
			if(meth == cp::Rpc::METH_RUN_CMD) {
				return 	"method: " + meth + "\n"
						"params: cmd_string OR [cmd_string, 1] OR [cmd_string, 2] OR [cmd_string, 1, 2]\n"
						"\tcmd_string: command with arguments to run\n"
						"\t1: remote process std_out will be set at this possition in returned list\n"
						"\t2: remote process std_err will be set at this possition in returned list\n"
						"Any param can be omited in params sa list, also param order is irrelevant, ie [1,\"ls\"] is valid param list."
						"return:\n"
						"\t* process stdout if param is just cmd_string\n"
						"\t* list of process exit code and std_out and std_err on same possitions as they are in params list.\n"
						;
			}
			else {
				return "No help for method: " + meth;
			}
		}
	}
	return Super::callMethod(shv_path, method, params);
}

shv::chainpack::RpcValue AppRootNode::processRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	if(rq.shvPath().toString().empty()) {
		if(rq.method() == cp::Rpc::METH_DEVICE_ID) {
			ShvAgentApp *app = ShvAgentApp::instance();
			const cp::RpcValue::Map& opts = app->rpcConnection()->connectionOptions().toMap();
			const cp::RpcValue::Map& dev = opts.value(cp::Rpc::KEY_DEVICE).toMap();
			//shvInfo() << dev[cp::Rpc::KEY_DEVICE_ID].toString();
			return dev.value(cp::Rpc::KEY_DEVICE_ID).toString();
		}
		if(rq.method() == cp::Rpc::METH_RUN_CMD) {
			ShvAgentApp *app = ShvAgentApp::instance();
			app->runCmd(rq, QString());
			return cp::RpcValue();
		}
		if(rq.method() == M_RUN_SCRIPT) {
			QTemporaryFile file;
			file.setAutoRemove(false);
			if (file.open()) {
				ShvAgentApp *app = ShvAgentApp::instance();
				shv::chainpack::RpcRequest rq2 = rq;
				std::string fn = file.fileName().toStdString();
				std::string cmd = "bash " + fn;
				std::string script;
				if(rq.params().isList()) {
					cp::RpcValue::List new_params;
					for(auto p : rq.params().toList()) {
						if(p.isString()) {
							script = p.toString();
							new_params.push_back(cmd);
						}
						else {
							int i = p.toInt();
							if(i == STDOUT_FILENO) {
								new_params.push_back(i);
							}
							else if(i == STDERR_FILENO) {
								new_params.push_back(i);
							}
							else {
								SHV_EXCEPTION("Invalid request parameter: " + p.toCpon());
							}
						}
					}
					rq2.setParams(new_params);
				}
				else {
					script = rq.params().toString();
					rq2.setParams(cmd);
				}
				file.write(script.data(), script.size());
				file.close();
				QFileDevice::Permissions perms = QFile::permissions(file.fileName());
				perms |= QFileDevice::QFileDevice::ExeOwner;
				QFile::setPermissions(file.fileName(), perms);
				/*
				if(0) {
					QString fn2 = "/tmp/test.sh";
					QFile f2(fn2);
					f2.open(QFile::WriteOnly);
					f2.write(script.data(), script.size());
					f2.close();
					QFileDevice::Permissions perms = QFile::permissions(f2.fileName());
					perms |= QFileDevice::QFileDevice::ExeOwner;
					QFile::setPermissions(f2.fileName(), perms);
					//rq2.setParams("bash /tmp/test.sh");
				}
				*/
				app->runCmd(rq2, file.fileName());
				return cp::RpcValue();
			}
			else {
				SHV_EXCEPTION("Cannot create temporary file for script to execute!");
			}
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
	//cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	m_rpcConnection = new si::rpc::DeviceConnection(this);

	if(!cli_opts->user_isset())
		cli_opts->setUser("iot");
	if(!cli_opts->password_isset())
		cli_opts->setPassword("lub42DUB");
	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &si::rpc::ClientConnection::brokerConnectedChanged, this, &ShvAgentApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &si::rpc::ClientConnection::rpcMessageReceived, this, &ShvAgentApp::onRpcMessageReceived);

	AppRootNode *root = new AppRootNode();
	m_shvTree = new si::node::ShvNodeTree(root, this);
	connect(m_shvTree->root(), &si::node::ShvRootNode::sendRpcMesage, m_rpcConnection, &si::rpc::ClientConnection::sendMessage);
	//m_shvTree->mkdir("sys/rproc");
	QString sys_fs_root_dir = QString::fromStdString(cli_opts->sysFsRootDir());
	if(!sys_fs_root_dir.isEmpty() && QDir(sys_fs_root_dir).exists()) {
		const char *SYS_FS = "sys/fs";
		shvInfo() << "Exporting" << sys_fs_root_dir << "as" << SYS_FS << "node";
		si::node::LocalFSNode *fsn = new si::node::LocalFSNode(sys_fs_root_dir);
		m_shvTree->mount(SYS_FS, fsn);
	}

	if(cliOptions()->connStatusUpdateInterval() > 0) {
		QTimer *tm = new QTimer(this);
		connect(tm, &QTimer::timeout, this, &ShvAgentApp::updateConnStatusFile);
		tm->start(cliOptions()->connStatusUpdateInterval() * 1000);
	}

	QTimer::singleShot(0, m_rpcConnection, &si::rpc::ClientConnection::open);
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
	cp::FindTunnelReqCtl find_tunnel_request;
	//find_tunnel_request.setCallerIds(rq.callerIds());
	shv::chainpack::RpcResponse resp = rq.makeResponse();
	resp.setTunnelCtl(find_tunnel_request);
	resp.setRegisterRevCallerIds();
	//resp.setResult(nullptr);
	shvDebug() << "Sending Find tunnel request:" << resp.toPrettyString();
	rpcConnection()->sendMessage(resp);

	si::rpc::RpcResponseCallBack *cb = new si::rpc::RpcResponseCallBack(resp.requestId().toInt(), this);
	connect(rpcConnection(), &si::rpc::ClientConnection::rpcMessageReceived, cb, &si::rpc::RpcResponseCallBack::onRpcMessageReceived);
	cp::RpcValue on_connected = rq.params();
	cb->start([this, on_connected](const shv::chainpack::RpcResponse &resp) {
		shvDebug() << "Received Find tunnel response:" << resp.toPrettyString();
		cp::TunnelCtl tctl1 = resp.tunnelCtl();
		if(tctl1.state() == cp::TunnelCtl::State::FindTunnelResponse) {
			cp::FindTunnelRespCtl find_tunnel_response(tctl1);
			find_tunnel_response.setRequestId(resp.requestId().toInt());
			SessionProcess *proc = new SessionProcess(this);
			QString app = QCoreApplication::applicationDirPath() + "/shvrexec";
			QStringList params;
			params << "--mtid";// << "-v" << "rpcrawmsg";
			shvInfo() << "starting child process:" << app << params.join(' ');
			proc->start(app, params);
			cp::RpcValue::Map startup_params;
			startup_params["findTunnelResponse"] = find_tunnel_response;
			startup_params["onConnectedCall"] = on_connected;
			std::string cpon = cp::RpcValue(std::move(startup_params)).toCpon();
			//shvInfo() << "cpon:" << cpon;
			proc->write(cpon.data(), cpon.size());
			proc->write("\n", 1);
		}
		else {
			shvError() << "Invalid response to FindTunnelRequest:" << resp.toPrettyString();
		}
	});
#if 0
	using ConnectionParams = si::rpc::ConnectionParams;
	using ConnectionParamsMT = si::rpc::ConnectionParams::MetaType;
	ConnectionParams conn_params;
	conn_params[ConnectionParamsMT::Key::Host] = m_rpcConnection->host();
	conn_params[ConnectionParamsMT::Key::Port] = m_rpcConnection->port();
	conn_params[ConnectionParamsMT::Key::User] = m_rpcConnection->user();
	conn_params[ConnectionParamsMT::Key::Password] = m_rpcConnection->password();
	cp::RpcValue::Map on_connected = rq.params().toMap();
	if(!on_connected.empty()) {
		on_connected[cp::Rpc::JSONRPC_REQUEST_ID] = rq.requestId();
		on_connected[cp::Rpc::JSONRPC_CALLER_ID] = rq.callerIds();
		conn_params[ConnectionParamsMT::Key::OnConnectedCall] = on_connected;
	}

	SessionProcess *proc = new SessionProcess(this);
	QString app = QCoreApplication::applicationDirPath() + "/shvrexec";
	QStringList params;
	params << "--mtid" << "-v" << "rpcrawmsg";
	shvInfo() << "starting child process:" << app << params.join(' ');
	proc->start(app, params);
	std::string cpon = conn_params.toRpcValue().toCpon();
	//shvInfo() << "cpon:" << cpon;
	proc->write(cpon.data(), cpon.size());
	proc->write("\n", 1);

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
			unsigned rexec_client_id = m.value(cp::Rpc::KEY_CLIENT_ID).toUInt();
			std::string rexec_client_broker_path = si::rpc::ClientConnection::brokerClientPath(rexec_client_id);
			std::string mount_point = m_rpcConnection->brokerMountPoint();
			size_t cnt = shv::core::StringView(mount_point).split('/').size();
			std::string rel_path;
			for (size_t i = 0; i < cnt; ++i) {
				rel_path = "../" + rel_path;
			}
			m[cp::Rpc::KEY_RELATIVE_PATH] = rel_path + rexec_client_broker_path;
			cp::RpcValue result = m;
			resp.setResult(result);
			shvInfo() << "Got tunnel handle from child process:" << result.toPrettyString();
		}
		catch (std::exception &e) {
			resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, e.what()));
		}
		m_rpcConnection->sendMessage(resp);
	});
#endif

}

void ShvAgentApp::runCmd(const shv::chainpack::RpcRequest &rq, QString file_to_remove)
{
	SessionProcess *proc = new SessionProcess(this);
	auto rq2 = rq;
	connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [this, proc, rq2, file_to_remove](int exit_code, QProcess::ExitStatus exit_status) {
		cp::RpcResponse resp = cp::RpcResponse::forRequest(rq2);
		if(exit_status == QProcess::CrashExit) {
			resp.setError(cp::RpcResponse::Error::createMethodCallExceptionError("Process crashed!"));
		}
		else {
			if(rq2.params().isList()) {
				cp::RpcValue::List lst;
				for(auto p : rq2.params().toList()) {
					if(p.isString()) {
						lst.push_back(exit_code);
					}
					else {
						int i = p.toInt();
						if(i == STDOUT_FILENO) {
							QByteArray ba = proc->readAllStandardOutput();
							lst.push_back(std::string(ba.constData(), ba.size()));
						}
						else if(i == STDERR_FILENO) {
							QByteArray ba = proc->readAllStandardError();
							lst.push_back(std::string(ba.constData(), ba.size()));
						}
					}
				}
				resp.setResult(lst);
			}
			else {
				QByteArray ba = proc->readAllStandardOutput();
				resp.setResult(std::string(ba.constData(), ba.size()));
			}
		}
		m_rpcConnection->sendMessage(resp);
		if(!file_to_remove.isEmpty())
			QFile::remove(file_to_remove);
	});
	connect(proc, &QProcess::errorOccurred, [this, rq2](QProcess::ProcessError error) {
		shvInfo() << "RunCmd: Exec process error:" << error;
		if(error == QProcess::FailedToStart) {
			cp::RpcResponse resp = cp::RpcResponse::forRequest(rq2);
			resp.setError(cp::RpcResponse::Error::createMethodCallExceptionError("Failed to start process"));
			m_rpcConnection->sendMessage(resp);
		}
	});
	std::string cmd;
	if(rq.params().isList()) {
		for(auto p : rq.params().toList()) {
			if(p.isString()) {
				cmd = p.toString();
			}
			else {
				int i = p.toInt();
				if(i == STDOUT_FILENO) {
				}
				else if(i == STDERR_FILENO) {
				}
				else {
					SHV_EXCEPTION("Invalid request parameter: " + p.toCpon());
				}
			}
		}
	}
	else {
		cmd = rq.params().toString();
	}
	shvDebug() << "CMD:" << cmd;
	proc->start(QString::fromStdString(cmd));
}

void ShvAgentApp::onBrokerConnectedChanged(bool is_connected)
{
	m_isBrokerConnected = is_connected;
}

void ShvAgentApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		shvInfo() << "RPC request received:" << rq.toPrettyString();
		if(m_shvTree->root()) {
			m_shvTree->root()->handleRpcRequest(rq);
		}
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rp(msg);
		shvInfo() << "RPC response received:" << rp.toPrettyString();
	}
	else if(msg.isSignal()) {
		cp::RpcSignal nt(msg);
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

void ShvAgentApp::updateConnStatusFile()
{
	QString fn = QString::fromStdString(cliOptions()->connStatusFile());
	if(fn.isEmpty())
		return;
	QFile f(fn);
	QDir dir = QFileInfo(f).dir();
	if(!dir.mkpath(dir.absolutePath())) {
		shvError() << "Cannot create directory:" << dir.absolutePath();
		return;
	}
	if(f.open(QFile::WriteOnly)) {
		f.write(m_isBrokerConnected? "1": "0", 1);
	}
	else {
		shvError() << "Cannot write to connection statu file:" << fn;
	}
}


