#include "revitestapp.h"
#include "appclioptions.h"
#include "lublicator.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/shvnodetree.h>
#include <shv/core/log.h>
#include <shv/chainpack/rpcdriver.h>

namespace cp = shv::chainpack;
namespace iot = shv::iotqt;

RevitestApp::RevitestApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	cp::RpcMessage::setMetaTypeImplicit(cli_opts->isMetaTypeImplicit());

	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);
	m_rpcConnection->setHost(cli_opts->serverHost().toStdString());
	m_rpcConnection->setPort(cli_opts->serverPort());
	m_rpcConnection->setProtocolType(shv::chainpack::Rpc::ProtocolType::Cpon);
	//m_clientConnection->setProfile("agent");
	cp::RpcValue::Map dev;
	dev["mount"] = "/test/lublin/odpojovace";
	dev["id"] = "123456";
	m_rpcConnection->setDevice(dev);
	m_rpcConnection->setUser("iot");
	m_revitest = new Revitest(this);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, m_revitest, &Revitest::onRpcMessageReceived);
	connect(m_revitest, &Revitest::sendRpcMessage, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::sendMessage);

	m_rpcConnection->open();
}

RevitestApp::~RevitestApp()
{
	shvInfo() << "destroying shv agent application";
}

