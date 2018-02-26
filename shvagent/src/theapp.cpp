#include "theapp.h"
#include "appclioptions.h"

#include <shv/iotqt/client/clientconnection.h>

#include <shv/coreqt/log.h>

namespace cp = shv::chainpack;

TheApp::TheApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv, cli_opts)
{
	shv::iotqt::client::ClientConnection *rpc = clientConnection();
	//rpc->setProfile("agent");
	//cp::RpcValue::Map dev;
	//dev["mount"] = "/test/shv/eu/pl/lublin/odpojovace";
	//dev["id"] = 123456;
	//rpc->setDevice(dev);
	rpc->setUser("iot");
	rpc->setProtocolVersion(shv::chainpack::Rpc::ProtocolVersion::ChainPack);
	connect(rpc, &shv::iotqt::client::ClientConnection::brokerConnectedChanged, this, &TheApp::onBrokerConnectedChanged);
	connect(rpc, &shv::iotqt::client::ClientConnection::messageReceived, this, &TheApp::onRpcMessageReceived);
}

TheApp::~TheApp()
{
	shvInfo() << "destroying shv agent application";
}

static void print_children(shv::iotqt::client::ClientConnection *rpc, const std::string &path, int indent)
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
		shv::iotqt::client::ClientConnection *rpc = clientConnection();
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
