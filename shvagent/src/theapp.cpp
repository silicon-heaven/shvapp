#include "theapp.h"
#include "appclioptions.h"

#include <shv/iotqt/rpc/clientconnection.h>

#include <shv/coreqt/log.h>

#include <QTimer>

namespace cp = shv::chainpack;

TheApp::TheApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	m_rpcConnection = new shv::iotqt::rpc::ClientConnection(this);
	m_rpcConnection->setHost(cli_opts->serverHost().toStdString());
	m_rpcConnection->setPort(cli_opts->serverPort());
	m_rpcConnection->setUser("iot");
	QString pv = cli_opts->protocolType();
	if(pv == QLatin1String("cpon"))
		m_rpcConnection->setProtocolType(shv::chainpack::Rpc::ProtocolType::Cpon);
	else if(pv == QLatin1String("jsonrpc"))
		m_rpcConnection->setProtocolType(shv::chainpack::Rpc::ProtocolType::JsonRpc);
	else
		m_rpcConnection->setProtocolType(shv::chainpack::Rpc::ProtocolType::ChainPack);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &TheApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &TheApp::onRpcMessageReceived);

	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
}

TheApp::~TheApp()
{
	shvInfo() << "destroying shv agent application";
}

static void print_children(shv::iotqt::rpc::ClientConnection *rpc, const std::string &path, int indent)
{
	//shvInfo() << "\tcall:" << "get" << "on shv path:" << shv_path;
	cp::RpcResponse resp = rpc->callShvMethodSync(path, cp::Rpc::METH_GET);
	shvDebug() << "\tgot response:" << resp.toCpon();
	if(resp.isError())
		throw shv::core::Exception(resp.error().message());
	shv::chainpack::RpcValue result = resp.result();
	if(result.isList()) {
		const cp::RpcValue::List list = resp.result().toList();
		for(const auto &dir : list) {
			std::string s = dir.toString();
			if(s.empty()) {
				shvError() << "empty dir name";
			}
			else {
				std::string path2 = path + '/' + s;
				shvInfo() << std::string(indent, ' ') << s;
				print_children(rpc, path2, indent + 2);
			}
		}
	}
	else {
		shvInfo() << std::string(indent, ' ') << result.toCpon();
	}
}

void TheApp::onBrokerConnectedChanged(bool is_connected)
{
	if(!is_connected)
		return;
	try {
		shvInfo() << "==================================================";
		shvInfo() << "   Lublicator Testing";
		shvInfo() << "==================================================";
		shv::iotqt::rpc::ClientConnection *rpc = m_rpcConnection;
		{
			shvInfo() << "------------ read shv tree";
			print_children(rpc, "", 0);
			//shvInfo() << "GOT:" << shv_path;
		}
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

void TheApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		shvInfo() << "RPC request received:" << rq.toCpon();
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rp(msg);
		shvInfo() << "RPC response received:" << rp.toCpon();
	}
	else if(msg.isNotify()) {
		cp::RpcNotify nt(msg);
		shvInfo() << "RPC notify received:" << nt.toCpon();
	}
}
