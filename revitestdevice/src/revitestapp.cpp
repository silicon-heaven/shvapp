#include "revitestapp.h"
#include "appclioptions.h"
#include "lublicator.h"

#include <shv/iotqt/client/connection.h>
#include <shv/iotqt/shvnodetree.h>
#include <shv/core/log.h>
#include <shv/chainpack/rpcdriver.h>

namespace cp = shv::chainpack;
namespace iot = shv::iotqt;

RevitestApp::RevitestApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv, cli_opts)
{
	m_clientConnection->setProtocolVersion(shv::chainpack::Rpc::ProtocolVersion::Cpon);
	//m_clientConnection->setProfile("agent");
	cp::RpcValue::Map dev;
	dev["mount"] = "/test/shv/eu/pl/lublin/odpojovace";
	dev["id"] = 123456;
	m_clientConnection->setDevice(dev);
	m_clientConnection->setUser("iot");
	createDevices();
}

RevitestApp::~RevitestApp()
{
	shvInfo() << "destroying shv agent application";
}

void RevitestApp::createDevices()
{
	m_devices = new iot::ShvNodeTree(this);
	static constexpr size_t LUB_CNT = 27;
	for (size_t i = 0; i < LUB_CNT; ++i) {
		auto *nd = new Lublicator(m_devices);
		nd->setNodeName(std::to_string(i+1));
	}
}
