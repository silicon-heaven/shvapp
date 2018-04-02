#include "shvagentapp.h"
#include "appclioptions.h"
#include "sessionprocess.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/node/localfsnode.h>

#include <shv/coreqt/log.h>

#include <QProcess>
#include <QTimer>

namespace cp = shv::chainpack;

static const char METH_OPEN_RSH[] = "openRsh";

shv::chainpack::RpcValue AppRootNode::dir(const shv::chainpack::RpcValue &methods_params)
{
	cp::RpcValue::List ret = Super::dir(methods_params).toList();
	ret.push_back(cp::Rpc::METH_APP_NAME);
	ret.push_back(METH_OPEN_RSH);
	return ret;
}

shv::chainpack::RpcValue AppRootNode::call(const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(method == cp::Rpc::METH_APP_NAME) {
		return QCoreApplication::instance()->applicationName().toStdString();
	}
	else if(method == METH_OPEN_RSH) {
		const shv::chainpack::RpcValue::Map &m = params.toMap();
		std::string proc_name = m.value("name").toString();
		ShvRExecApp *app = ShvRExecApp::instance();
		return app->openRsh(proc_name);
	}
	return Super::call(method, params);
}

ShvRExecApp::ShvRExecApp(int &argc, char **argv, AppCliOptions* cli_opts)
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

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &ShvRExecApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvRExecApp::onRpcMessageReceived);

	AppRootNode *root = new AppRootNode();
	m_shvTree = new shv::iotqt::node::ShvNodeTree(root, this);
	m_shvTree->mkdir("sys");
	QString sys_fs_root_dir = cli_opts->sysFsRootDir();
	if(!sys_fs_root_dir.isEmpty() && QDir(sys_fs_root_dir).exists()) {
		const char *SYS_FS = "sys/fs";
		shvInfo() << "Exporting" << sys_fs_root_dir << "as" << SYS_FS << "node";
		shv::iotqt::node::LocalFSNode *fsn = new shv::iotqt::node::LocalFSNode(sys_fs_root_dir);
		m_shvTree->mount(SYS_FS, fsn);
	}

	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
}

ShvRExecApp::~ShvRExecApp()
{
	shvInfo() << "destroying shv agent application";
}

ShvRExecApp *ShvRExecApp::instance()
{
	return qobject_cast<ShvRExecApp *>(QCoreApplication::instance());
}

shv::chainpack::RpcValue ShvRExecApp::openRsh(const std::string &name)
{
	QString qname = QString::fromStdString(name);
	{
		QProcess*p = findChild<QProcess*>(qname);
		if(p) {
			shvWarning() << "Process with name:" << qname << "exists already, it will be canceled.";
			p->close();
			delete p;
		}
	}
	shv::iotqt::rpc::DeviceConnection *conn = m_rpcConnection;
	std::string mount_point = ".broker/clients/" + std::to_string(conn->brokerClientId()) + "/rproc/" + name;
	QString app = QCoreApplication::applicationDirPath() + "/shvrexec";
	QStringList params;
	SessionProcess *proc = new SessionProcess(this);
	params << "-s" << QString::fromStdString(conn->host());
	params << "-p" << QString::number(conn->port());
	params << "-u" << QString::fromStdString(conn->user());
	params << "-m" << QString::fromStdString(mount_point);
	cp::RpcValue::Map st;
	st["clientId"] = m_rpcConnection->brokerClientId();
	st["sessionId"] = (uint64_t)proc;
	std::string session_token = cp::Utils::toHex(cp::RpcValue(st).toChainPack());
	params << "-st" << QString::fromStdString(session_token);
	params << "-e" << "/bin/sh";
	shvInfo() << "starting child process:" << app << params.join(' ');
	proc->start(app, params);
	proc->write(conn->password().data(), conn->password().size());
	proc->write("\n", 1);
	cp::RpcValue::Map ret;
	ret["mount"] = mount_point;
	return ret;
}

void ShvRExecApp::onBrokerConnectedChanged(bool is_connected)
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

void ShvRExecApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		cp::RpcResponse resp = cp::RpcResponse::forRequest(rq);
		try {
			//shvInfo() << "RPC request received:" << rq.toCpon();
			const std::string shv_path = rq.shvPath();
			std::string path_rest;
			shv::iotqt::node::ShvNode *nd = m_shvTree->cd(shv_path, &path_rest);
			if(!nd)
				SHV_EXCEPTION("Path not found: " + shv_path);
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

