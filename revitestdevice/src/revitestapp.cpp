#include "revitestapp.h"
#include "appclioptions.h"
#include "lublicator.h"

#include <shv/iotqt/client/clientconnection.h>
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
	m_revitest = new Revitest(this);
	connect(m_clientConnection, &shv::iotqt::client::ClientConnection::messageReceived, m_revitest, &Revitest::onRpcMessageReceived);
	connect(m_revitest, &Revitest::sendRpcMessage, m_clientConnection, &shv::iotqt::client::ClientConnection::sendMessage);
}

RevitestApp::~RevitestApp()
{
	shvInfo() << "destroying shv agent application";
}

