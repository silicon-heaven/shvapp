#include "revitestapp.h"
#include "appclioptions.h"
#include "lublicator.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/core/log.h>
#include <shv/chainpack/rpcdriver.h>

namespace cp = shv::chainpack;
namespace iot = shv::iotqt;

RevitestApp::RevitestApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);

	if(!cli_opts->mountPoint_isset())
		cli_opts->setMountPoint("/test/lublin/odpojovace");
	m_rpcConnection->setCliOptions(cli_opts);

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

