#include "theapp.h"
#include "appclioptions.h"

#include <shv/iotqt/client/connection.h>

#include <shv/coreqt/log.h>

namespace cp = shv::chainpack;

TheApp::TheApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv, cli_opts)
{
	shv::iotqt::client::Connection *rpc = clientConnection();
	//rpc->setProfile("agent");
	//cp::RpcValue::Map dev;
	//dev["mount"] = "/test/shv/eu/pl/lublin/odpojovace";
	//dev["id"] = 123456;
	//rpc->setDevice(dev);
	rpc->setUser("iot");
	rpc->setProtocolVersion(shv::chainpack::Rpc::ProtocolVersion::ChainPack);
	connect(rpc, &shv::iotqt::client::Connection::brokerConnectedChanged, this, &TheApp::onBrokerConnectedChanged);
}

TheApp::~TheApp()
{
	shvInfo() << "destroying shv agent application";
}

static void print_children(shv::iotqt::client::Connection *rpc, const std::string &path, int indent)
{
	std::string shv_path = path;
	//shvInfo() << "\tcall:" << "get" << "on shv path:" << shv_path;
	cp::RpcResponse resp = rpc->callShvMethodSync(shv_path, cp::Rpc::METH_GET);
	//shvInfo() << "\tgot response:" << resp.toStdString();
	if(resp.isError())
		throw shv::core::Exception(resp.error().message());
	const cp::RpcValue::List list = resp.result().toList();
	for(auto dir : list) {
		shv_path += '/' + dir.toString();
		shvInfo() << std::string(indent, ' ') << dir.toString();
		print_children(rpc, shv_path, indent + 2);
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
		shv::iotqt::client::Connection *rpc = clientConnection();
		{
			shvInfo() << "------------ read shv tree";
			print_children(rpc, "", 0);
			//shvInfo() << "GOT:" << shv_path;
		}
#if 0
		{
			shvInfo() << "------------ set battery level";
			QString shv_path_lubl = "/shv/eu/pl/lublin/odpojovace/15/";
			for(auto prop : {"status", "batteryLimitLow", "batteryLimitHigh", "batteryLevelSimulation"}) {
				QString shv_path = shv_path_lubl + prop;
				cp::RpcResponse resp = rpc->callShvMethodSync(shv_path, cp::RpcMessage::METHOD_GET);
				shvInfo() << "\tproperty" << prop << ":" << resp.result().toStdString();
			}
			{
				QString shv_path = shv_path_lubl + "batteryLevelSimulation";
				for (int val = 200; val < 260; val += 5) {
					cp::RpcValue::Decimal dec_val(val, 1);
					shvInfo() << "\tcall:" << "set" << dec_val.toString() << "on shv path:" << shv_path;
					cp::RpcResponse resp = rpc->callShvMethodSync(shv_path, cp::RpcMessage::METHOD_SET, dec_val);
					shvInfo() << "\tgot response:" << resp.toStdString();
					if(resp.isError())
						throw shv::core::Exception(resp.error().message());
				}
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
