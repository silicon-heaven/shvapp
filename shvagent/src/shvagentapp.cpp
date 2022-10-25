#include "shvagentapp.h"
#include "appclioptions.h"
#include "sessionprocess.h"
#include "testernode.h"

#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/node/localfsnode.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/tunnelctl.h>
#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/cponreader.h>

#include <shv/core/stringview.h>

#include <QProcess>
#include <QSocketNotifier>
#include <QTimer>
#include <QtGlobal>
#include <QFileInfo>
#include <QRegularExpression>
#include <QCryptographicHash>

#include <fstream>

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

#define logRunCmd() shvCMessage("RunCmd")

#define logTesterD() shvCDebug("Tester")
#define logTesterI() shvCInfo("Tester")
#define logTesterW() shvCWarning("Tester")
#define logTesterE() shvCError("Tester")
#define logTesterPassed(test_no) shvCError("Tester").color(NecroLog::Color::LightGreen) << "#" << test_no << "[PASSED]"
#define logTesterFailed(test_no) shvCError("Tester").color(NecroLog::Color::LightRed) << "#" << test_no << "[FAILED]"

using namespace std;

namespace cp = shv::chainpack;
namespace si = shv::iotqt;

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
	//{cp::Rpc::METH_CONNECTION_TYPE, cp::MetaMethod::Signature::RetVoid, 0, cp::Rpc::GRANT_BROWSE},
	{cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_DEVICE_TYPE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
	//{cp::Rpc::METH_HELP, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_RUN_CMD, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_COMMAND,
		"Parameter can be string or list\n"
		"When list is provided: [\"command\", [\"arg1\", ... ], 1, 2, {\"ENV_VAR1\": \"value\", ...}]\n"
		"\t list of values is returned\n"
		"\t order and number of parameters list items is arbitrary, only the command part part is mandatory\n"
		"\t - string interpretted as CMD, process return value is appended to the retval list\n"
		"\t - list is interpretted as launched process argument list\n"
		"\t - int is interpretted as 1 == stdout, 2 == stderr, process stdin/stderr content  is appended to the retval list\n"
		"\t - map is interpretted as launched process environment\n"
	},
	{cp::Rpc::METH_RUN_SCRIPT, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_COMMAND,
		"Parameter can be string or list\n"
		"When list is provided: [\"script-content\", [\"arg1\", ... ], 1, 2, {\"ENV_VAR1\": \"value\", ...}]\n"
		"\t list of same size is returned\n"
		"\t script-content is written to temporrary file before run, script-content SHA1 can be used instead of script-content if one wants to run same script again.\n"
		"\t order and number of parameters list items is arbitrary, only the script-content part part is mandatory\n"
		"\t - string is interpretted as script-content, process return value is appended to the retval list\n"
		"\t - list is interpretted as launched process argument list\n"
		"\t - int is interpretted as 1 == stdout, 2 == stderr, process stdin/stderr content  is appended to the retval list\n"
		"\t - map is interpretted as launched process environment\n"
	},
	{cp::Rpc::METH_LAUNCH_REXEC, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_COMMAND},
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

shv::chainpack::RpcValue AppRootNode::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == cp::Rpc::METH_APP_NAME) {
			return QCoreApplication::instance()->applicationName().toStdString();
		}
		/*
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
		*/
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

shv::chainpack::RpcValue AppRootNode::callMethodRq(const shv::chainpack::RpcRequest &rq)
{
	if(rq.shvPath().asString().empty()) {
		if(rq.method() == cp::Rpc::METH_DEVICE_ID) {
			ShvAgentApp *app = ShvAgentApp::instance();
			const cp::RpcValue::Map& opts = app->rpcConnection()->connectionOptions().toMap();
			const cp::RpcValue::Map& dev = opts.value(cp::Rpc::KEY_DEVICE).toMap();
			//shvInfo() << dev[cp::Rpc::KEY_DEVICE_ID].toString();
			return dev.value(cp::Rpc::KEY_DEVICE_ID).toString();
		}
		if(rq.method() == cp::Rpc::METH_DEVICE_TYPE) {
			return "ShvAgent";
		}
		if(rq.method() == cp::Rpc::METH_RUN_CMD) {
			ShvAgentApp *app = ShvAgentApp::instance();
			app->runCmd(rq);
			return cp::RpcValue();
		}
		if(rq.method() == cp::Rpc::METH_RUN_SCRIPT) {
			QByteArray script;
			cp::RpcValue::List args;
			if(rq.params().isList()) {
				for(auto p : rq.params().toList()) {
					if(p.isString())
						script = QByteArray::fromStdString(p.asString());
				}
			}
			else {
				script = QByteArray::fromStdString(rq.params().asString());
			}

			ShvAgentApp *app = ShvAgentApp::instance();
			const cp::RpcValue::Map& opts = app->rpcConnection()->connectionOptions().toMap();
			const cp::RpcValue::Map& dev = opts.value(cp::Rpc::KEY_DEVICE).toMap();
			auto device_id = dev.value(cp::Rpc::KEY_DEVICE_ID).toString();
			auto script_dir = QString::fromStdString("/tmp/shvagent/" + device_id + "/scripts/");
			QByteArray sha1;
			/// SHA1 40 chars long
			QRegularExpression rx("^[0-9a-fA-F]{40}$");
			bool is_sha1 = rx.match(script).hasMatch();
			if(is_sha1) {
				sha1 = script;
				if(!QFile::exists(script_dir + sha1))
					throw cp::RpcException(cp::RpcResponse::Error::UserCode, "SHA1_NOT_EXISTS - Script with SHA1 privided doesn't exist, sha1: " + script.toStdString());
			}
			else {
				QCryptographicHash ch(QCryptographicHash::Sha1);
				ch.addData(script);
				sha1 = ch.result().toHex();
				QDir().mkpath(script_dir);
				QFile f(script_dir + sha1);
				if(f.open(QFile::WriteOnly)) {
					f.write(script);
				}
				else {
					SHV_EXCEPTION("Cannot create script file for script to execute, file: " + (script_dir + sha1).toStdString());
				}
			}

			shv::chainpack::RpcRequest rq2 = rq;
			std::string cmd = "bash";
			std::string fn = (script_dir + sha1).toStdString();
			args.insert(args.begin(), fn);
			//std::string cmd = "bash " + fn;
			cp::RpcValue::List new_params;
			new_params.push_back(cmd);
			new_params.push_back(args);
			if(rq.params().isList()) {
				for(auto p : rq.params().toList()) {
					if(p.isMap()) {
						new_params.push_back(p);
					}
					else if(p.isInt()) {
						int i = p.toInt();
						if(i == STDOUT_FILENO) {
							new_params.push_back(i);
						}
						else if(i == STDERR_FILENO) {
							new_params.push_back(i);
						}
						else {
							SHV_EXCEPTION("Invalid request output stream parameter: " + p.toCpon());
						}
					}
				}
			}
			rq2.setParams(new_params);
			//QFileDevice::Permissions perms = QFile::permissions(file.fileName());
			//perms |= QFileDevice::QFileDevice::ExeOwner;
			//QFile::setPermissions(file.fileName(), perms);
			app->runCmd(rq2, rq.params().isString());
			return cp::RpcValue();
		}
		if(rq.method() == cp::Rpc::METH_LAUNCH_REXEC) {
			ShvAgentApp *app = ShvAgentApp::instance();
			app->launchRexec(rq);
			return cp::RpcValue();
		}
	}
	return Super::callMethodRq(rq);
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
//#ifdef Q_OS_UNIX
//	if(0 != ::setpgid(0, 0))
//		shvError() << "Cannot make shvagent process the group leader, error set process group ID:" << errno << ::strerror(errno);
//#endif
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
	connect(m_shvTree->root(), &si::node::ShvRootNode::sendRpcMessage, m_rpcConnection, &si::rpc::ClientConnection::sendMessage);
	//m_shvTree->mkdir("sys/rproc");
	QString sys_fs_root_dir = QString::fromStdString(cli_opts->sysFsRootDir());
	if(!sys_fs_root_dir.isEmpty() && QDir(sys_fs_root_dir).exists()) {
		const char *SYS_FS = "sys/fs";
		shvInfo() << "Exporting" << sys_fs_root_dir << "as" << SYS_FS << "node";
		si::node::LocalFSNode *fsn = new si::node::LocalFSNode(sys_fs_root_dir);
		m_shvTree->mount(SYS_FS, fsn);
	}
	if(cli_opts->isTesterEnabled() || !cli_opts->testerScript().empty()) {
		const char *TESTER_DIR = "tester";
		shvInfo() << "Enabling tester at:" << TESTER_DIR;
		TesterNode *nd = new TesterNode(nullptr);
		m_shvTree->mount(TESTER_DIR, nd);
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
	cb->start([this, on_connected](const shv::chainpack::RpcResponse &find_tunnel_resp) {
		shvDebug() << "Received Find tunnel response:" << find_tunnel_resp.toPrettyString();
		cp::TunnelCtl tctl1 = find_tunnel_resp.tunnelCtl();
		if(tctl1.state() == cp::TunnelCtl::State::FindTunnelResponse) {
			cp::FindTunnelRespCtl find_tunnel_response(tctl1);
			find_tunnel_response.setRequestId(find_tunnel_resp.requestId().toInt());
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
			proc->write(cpon.data(), static_cast<unsigned>(cpon.size()));
			proc->write("\n", 1);
		}
		else {
			shvError() << "Invalid response to FindTunnelRequest:" << find_tunnel_resp.toPrettyString();
		}
	});
}

void ShvAgentApp::runCmd(const shv::chainpack::RpcRequest &rq, bool std_out_only)
{
	SessionProcess *proc = new SessionProcess(this);
	auto rq2 = rq;
	connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, proc, &QProcess::kill);
	connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [this, proc, rq2, std_out_only](int exit_code, QProcess::ExitStatus exit_status) {
		logRunCmd() << "Proces finished, exit code:" << exit_code << "exit status:" << exit_status;
		cp::RpcResponse resp = cp::RpcResponse::forRequest(rq2);
		if(exit_status == QProcess::CrashExit) {
			logRunCmd() << "Proces crashed!";
			resp.setError(cp::RpcResponse::Error::createMethodCallExceptionError("Process crashed!"));
		}
		else {
			if(rq2.params().isList() && !std_out_only) {
				cp::RpcValue::List lst;
				for(auto p : rq2.params().toList()) {
					if(p.isString()) {
						lst.push_back(exit_code);
					}
					else if(p.isInt()) {
						int i = p.toInt();
						if(i == STDOUT_FILENO) {
							QByteArray ba = proc->readAllStandardOutput();
							shvDebug() << "\t stdout:" << ba.toStdString();
							lst.push_back(std::string(ba.constData(), static_cast<unsigned>(ba.size())));
						}
						else if(i == STDERR_FILENO) {
							QByteArray ba = proc->readAllStandardError();
							shvDebug() << "\t stderr:" << ba.toStdString();
							lst.push_back(std::string(ba.constData(), static_cast<unsigned>(ba.size())));
						}
					}
				}
				resp.setResult(lst);
				logRunCmd() << "Proces exit OK, result:" << resp.result().toCpon();
			}
			else {
				QByteArray ba = proc->readAllStandardOutput();
				shvDebug() << "\t stdout:" << ba.toStdString();
				resp.setResult(std::string(ba.constData(), static_cast<unsigned>(ba.size())));
				logRunCmd() << "Proces exit OK, result:" << resp.result().toCpon();
			}
		}
		m_rpcConnection->sendMessage(resp);
	});
	connect(proc, &QProcess::errorOccurred, [this, proc, rq2](QProcess::ProcessError error) {
		shvInfo() << "RunCmd: Exec process error:" << error;
		if(error == QProcess::FailedToStart) {
			cp::RpcResponse resp = cp::RpcResponse::forRequest(rq2);
			resp.setError(cp::RpcResponse::Error::createMethodCallExceptionError("Failed to start process (file not found, resource error)"));
			m_rpcConnection->sendMessage(resp);
			proc->deleteLater();
		}
	});
	std::string cmd;
	QStringList args;
	QProcessEnvironment env;
	if(rq.params().isList()) {
		for(auto p : rq.params().toList()) {
			if(p.isString()) {
				cmd = p.toString();
			}
			else if(p.isList()) {
				for(auto kv : p.toList())
					args << QString::fromStdString(kv.asString());
			}
			else if(p.isMap()) {
				for(auto kv : p.toMap())
					env.insert(QString::fromStdString(kv.first), QString::fromStdString(kv.second.toStdString()));
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
	logRunCmd() << "CMD:" << cmd << args.join(' ') << "env:" << env.toStringList().join(',');
	proc->setProcessEnvironment(env);
	proc->start(QString::fromStdString(cmd), args);
}

void ShvAgentApp::onBrokerConnectedChanged(bool is_connected)
{
	m_isBrokerConnected = is_connected;
	AppCliOptions *cli_opts = cliOptions();
	if(!cli_opts->testerScript().empty()) {
		fstream is(cli_opts->testerScript());
		if(!is)
			SHV_EXCEPTION("Cannot open file " + cli_opts->testerScript() + " for reading");
		cp::CponReader rd(is);
		m_testerScript = rd.read();
		tester_processShvCalls();
	}
}

void ShvAgentApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		shvDebug() << "RPC request received:" << rq.toPrettyString();
		if(m_shvTree->root()) {
			m_shvTree->root()->handleRpcRequest(rq);
		}
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rp(msg);
		shvDebug() << "RPC response received:" << rp.toPrettyString();
	}
	else if(msg.isSignal()) {
		cp::RpcSignal nt(msg);
		shvDebug() << "RPC notify received:" << nt.toPrettyString();
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

void ShvAgentApp::tester_processShvCalls()
{
	if(!m_rpcConnection->isBrokerConnected()) {
		return;
	}
	const shv::chainpack::RpcValue::List &tests = m_testerScript.toList();
	if(m_currentTestIndex >= tests.size())
		return;

	cp::RpcValue rv = tests.at(m_currentTestIndex++);
	const shv::chainpack::RpcValue::Map &task = rv.toMap();
	std::string descr = task.value("descr").toString();
	std::string cmd = task.value("cmd").toString();

	unsigned on_success_skip = task.value("onSuccess").toMap().value("skip", 1).toUInt();
	unsigned on_success_ix = m_currentTestIndex + on_success_skip - 1;
	auto on_success_fn = [this, on_success_skip, on_success_ix]() {
		if(on_success_skip > 0) {
			m_currentTestIndex = on_success_ix;
			QTimer::singleShot(0, this, &ShvAgentApp::tester_processShvCalls);
		}
	};
	auto on_error_fn = [this]() {
		m_currentTestIndex = numeric_limits<decltype (m_currentTestIndex)>::max();
	};

	auto test_no = m_currentTestIndex;
	string test_id = task.value("id").toString();
	if(test_id.empty())
		test_id = "TEST_STEP_" + to_string(test_no);
	logTesterI() << "=========================================";
	logTesterI() << "#" << test_no << "ID:" << test_id << "COMMAND:" << cmd;
	logTesterI() << "DESCR:" << descr;
	if(cmd == "call") {
		std::string shv_path = task.value("shvPath").toString();
		std::string method = task.value("method").toString();
		cp::RpcValue params = task.value("params");
		cp::RpcValue result = task.value("result");

		int rq_id = m_rpcConnection->nextRequestId();
		logTesterD() << "CALL rqid:" << rq_id << "msg:" << rv.toCpon();
		shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rq_id, this);
		cb->start(this, [rq_id, result,
				  on_success_fn,
				  on_error_fn,
				  test_no](const cp::RpcResponse &resp) {
			if(resp.isValid()) {
				if(resp.isError()) {
					logTesterW() << "ERROR rqid:" << rq_id << "request error:" << resp.error().toString();
					logTesterFailed(test_no) << "call response error";
					on_error_fn();
				}
				else {
					logTesterD() << "RESULT rqid:" << rq_id << "result:" << resp.toCpon();
					if(resp.result() == result) {
						logTesterPassed(test_no);
						on_success_fn();
					}
					else {
						logTesterFailed(test_no) << "wrong result" << resp.result().toCpon();
						on_error_fn();
					}
				}
			}
			else {
				logTesterE() << "Invalid response rqid:" << rq_id;
				on_error_fn();
			}
		});
		m_rpcConnection->callShvMethod(rq_id, shv_path, method, params);
	}
	else if(cmd == "sigTrap") {
		std::string shv_path = task.value("shvPath").toString();
		std::string method = task.value("method").toString();
		cp::RpcValue params = task.value("params");
		int timeout = task.value("timeout").toInt();
		QObject *ctx = new QObject();
		QTimer::singleShot(timeout, ctx, [ctx, on_error_fn, timeout, test_no]() {
			logTesterFailed(test_no) << "timeout after:" << timeout;
			ctx->deleteLater();
			on_error_fn();
		});
		connect(m_rpcConnection
				, &shv::iotqt::rpc::DeviceConnection::rpcMessageReceived
				, ctx
				, [ctx, on_success_fn, shv_path, method, params, test_no](const shv::chainpack::RpcMessage &msg) {
			logTesterD() << "MSG:" << msg.toCpon();
			if(msg.isSignal()) {
				cp::RpcSignal sig(msg);
				if(sig.shvPath() == shv_path && sig.method() == method && sig.params() == params) {
					logTesterPassed(test_no);
					ctx->deleteLater();
					on_success_fn();
				}
			}
		});
		QTimer::singleShot(0, this, &ShvAgentApp::tester_processShvCalls);
	}
}


