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
#include <QTimer>

namespace cp = shv::chainpack;

static const char METH_OPEN_RSH[] = "openRsh";

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, false},
	{cp::Rpc::METH_CONNECTION_TYPE, cp::MetaMethod::Signature::RetVoid, false},
	{METH_OPEN_RSH, cp::MetaMethod::Signature::RetVoid, false},
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
	if(rq.method() == METH_OPEN_RSH) {
		ShvAgentApp *app = ShvAgentApp::instance();
		app->openRsh(rq);
		return cp::RpcValue();
	}
	return Super::processRpcRequest(rq);
}

ShvAgentApp::ShvAgentApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
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

void ShvAgentApp::openRsh(const shv::chainpack::RpcRequest &rq)
{
	//const shv::chainpack::RpcValue::Map &m = rq.params().toMap();
	/*
	std::string tun_name;// = m.value("name").toString();
	if(tun_name.empty()) {
		static int n = 0;
		tun_name = "proc" + std::to_string(++n);
	}
	QString qname = QString::fromStdString(name);
	{
		QProcess*p = findChild<SessionProcess*>(qname);
		if(p) {
			shvWarning() << "Process with name:" << name << "exists already.";
			//p->close();
			//delete p;
			cp::RpcResponse resp = cp::RpcResponse::forRequest(rq);
			resp.setError(cp::RpcResponse::Error::create(
							  cp::RpcResponse::Error::MethodInvocationException,
							  "Process with name: " + name + " exists already."));
			m_shvTree->root()->emitSendRpcMesage(resp);
			return;
		}
	}
	*/
	using TunnelParams = shv::iotqt::rpc::TunnelParams;
	using TunnelParamsMT = shv::iotqt::rpc::TunnelParams::MetaType;
	TunnelParams tun_params;
	tun_params[TunnelParamsMT::Key::Host] = m_rpcConnection->host();
	tun_params[TunnelParamsMT::Key::Port] = m_rpcConnection->port();
	tun_params[TunnelParamsMT::Key::User] = m_rpcConnection->user();
	tun_params[TunnelParamsMT::Key::Password] = m_rpcConnection->password();
	tun_params[TunnelParamsMT::Key::ParentClientId] = m_rpcConnection->brokerClientId();
	tun_params[TunnelParamsMT::Key::CallerClientIds] = rq.callerIds();
	//tun_params[TunnelParamsMT::Key::TunnelResponseRequestId] = rq.requestId();
	//tun_params[TunnelParamsMT::Key::TunName] = tun_name;
	//std::string mount_point = ".broker/clients/" + std::to_string(m_rpcConnection->brokerClientId()) + "/rproc/" + name;
	QString app = QCoreApplication::applicationDirPath() + "/shvrexec";
	QStringList params;
	SessionProcess *proc = new SessionProcess(this);
	//proc->setCurrentReadChannel(QProcess::StandardOutput);
	auto rq2 = rq;
	connect(proc, &SessionProcess::readyReadStandardOutput, [this, proc, rq2]() {
		if(!proc->canReadLine()) {
			return;
		}
		cp::RpcResponse resp = cp::RpcResponse::forRequest(rq2);
		try {
			QByteArray ba = proc->readLine();
			std::string data(ba.constData(), ba.size());
			cp::RpcValue::Map m = cp::RpcValue::fromCpon(data).toMap();
			cp::RpcValue::Map result;
			result[cp::Rpc::KEY_TUNNEL_HANDLE] = m.value(cp::Rpc::KEY_TUNNEL_HANDLE);
			resp.setResult(result);
		}
		catch (std::exception &e) {
			resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodInvocationException, e.what()));
		}
		shvInfo() << "Process started, sending response:" << resp.toPrettyString();
		m_rpcConnection->sendMessage(resp);
	});
	/*
	params << "-s" << QString::fromStdString(m_rpcConnection->host());
	params << "-p" << QString::number(m_rpcConnection->port());
	params << "-u" << QString::fromStdString(m_rpcConnection->user());
	params << "-m" << QString::fromStdString(mount_point);
	cp::RpcValue::Map st;
	st["clientId"] = m_rpcConnection->brokerClientId();
	st["sessionId"] = (uint64_t)proc;
	std::string session_token = cp::Utils::toHex(cp::RpcValue(st).toChainPack());
	params << "-st" << QString::fromStdString(session_token);
	*/
	params << "-e" << "/bin/sh";
	shvInfo() << "starting child process:" << app << params.join(' ');
	proc->start(app, params);
	std::string cpon = tun_params.toRpcValue().toCpon();
	proc->write(cpon.data(), cpon.size());
	proc->write("\n", 1);
}

void ShvAgentApp::onBrokerConnectedChanged(bool is_connected)
{
	if(!is_connected)
		return;
	try {
#if 0
		shvInfo() << "==================================================";
		shvInfo() << "   Lublicator Testing";
		shvInfo() << "==================================================";
		shv::iotqt::rpc::ClientConnection *rpc = m_rpcConnection;
		if(0) {
			shvInfo() << "------------ read shv tree";
			print_children(rpc, "", 0);
			//shvInfo() << "GOT:" << shv_path;
		}
		bool on = 0;
		bool off = !on;
		{
			std::string shv_path = "/test/shv/lublicator2/";
			shvInfo() << "------------ get STATUS";
			cp::RpcResponse resp = rpc->callShvMethodSync(shv_path + "status", cp::Rpc::METH_GET);
			shvInfo() << "\tgot response:" << resp.toCpon();
			shv::chainpack::RpcValue result = resp.result();
			shvInfo() << result.toCpon() << lublicatorStatusToString(result.toUInt());
			on = !(result.toUInt() & (unsigned)LublicatorStatus::PosOn);
			off = !on;
		}
		if(on) {
			shvInfo() << "------------ switch ON";
			std::string shv_path = "/test/shv/lublicator2/";
			cp::RpcResponse resp = rpc->callShvMethodSync(shv_path + "cmdOn", cp::Rpc::METH_SET, true);
			shvInfo() << "\tgot response:" << resp.toCpon();
		}
		if(off) {
			shvInfo() << "------------ switch OFF";
			std::string shv_path = "/test/shv/lublicator2/";
			cp::RpcResponse resp = rpc->callShvMethodSync(shv_path + "cmdOff", cp::Rpc::METH_SET, true);
			shvInfo() << "\tgot response:" << resp.toCpon();
		}
		return;
		{
			shvInfo() << "------------ get battery level";
			std::string shv_path_lubl = "/test/shv/eu/pl/lublin/odpojovace/15/";
			shvInfo() << shv_path_lubl;
			for(auto prop : {"status", "batteryLimitLow", "batteryLimitHigh", "batteryLevelSimulation"}) {
				std::string shv_path = shv_path_lubl + prop;
				cp::RpcResponse resp = rpc->callShvMethodSync(shv_path, cp::Rpc::METH_GET);
				shvInfo() << "\tproperty" << prop << ":" << resp.result().toCpon();
			}
			for (int val = 200; val < 260; val += 5) {
				std::string shv_path = shv_path_lubl + "batteryLevelSimulation";
				cp::RpcValue::Decimal dec_val(val, 1);
				shvInfo() << "\tcall:" << "set" << dec_val.toString() << "on shv path:" << shv_path;
				cp::RpcResponse resp = rpc->callShvMethodSync(shv_path, cp::Rpc::METH_SET, dec_val);
				shvInfo() << "\tgot response:" << resp.toCpon();
				if(resp.isError())
					throw shv::core::Exception(resp.error().message());
				QCoreApplication::processEvents();
			}
		}
#endif
#if 0
		{
			{
				QString shv_path = shv_path_lubl + "batteryLevelSimulation";
			}
			for(auto prop : {"status", "batteryLimitLow", "batteryLimitHigh", "batteryLevelSimulation"}) {
				QString shv_path = shv_path_lubl + prop;
				cp::RpcResponse resp = rpc->callShvMethodSync(shv_path, cp::RpcMessage::METHOD_GET);
				shvInfo() << "\tproperty" << prop << ":" << resp.result().toStdString();
			}
			{
				QString shv_path = shv_path_lubl + "batteryLevelSimulation";
				rpc->callShvMethodSync(shv_path, cp::RpcMessage::METHOD_SET, cp::RpcValue::Decimal(240, 1));
			}
		}
#endif
	}
	catch (shv::core::Exception &e) {
		shvError() << "Lublicator Testing FAILED:" << e.message();
	}
}

void ShvAgentApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
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
		shvInfo() << "RPC response received:" << rp.toCpon();
	}
	else if(msg.isNotify()) {
		cp::RpcNotify nt(msg);
		shvInfo() << "RPC notify received:" << nt.toCpon();
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
