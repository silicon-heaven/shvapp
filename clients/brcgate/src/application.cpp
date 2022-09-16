#include "application.h"
#include "appclioptions.h"
#include "revitestdevice.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/core/utils/shvfilejournal.h>
#include <shv/core/utils/shvlogtypeinfo.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/core/log.h>
#include <shv/chainpack/rpcdriver.h>

#include <QFile>
#include <QTimer>

#define logTestCallsD() nCDebug("TestCalls")
#define logTestCallsI() nCInfo("TestCalls")
#define logTestCallsW() nCWarning("TestCalls")
#define logTestCallsE() nCError("TestCalls")
#define logTestPassed(test_no) nCError("TestCalls").color(NecroLog::Color::LightGreen) << "#" << test_no << "[PASSED]"
#define logTestFailed(test_no) nCError("TestCalls").color(NecroLog::Color::LightRed) << "#" << test_no << "[FAILED]"

namespace cp = shv::chainpack;
namespace iot = shv::iotqt;

Application::Application(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);

	if(!cli_opts->user_isset())
		cli_opts->setUser("iot");
	if(!cli_opts->password_isset())
		cli_opts->setPassword("lub42DUB");
	if(!cli_opts->mountPoint_isset())
		cli_opts->setMountPoint("/test/lublin/odpojovace");
	m_rpcConnection->setCliOptions(cli_opts);

	//connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, m_revitest, &RevitestDevice::onRpcMessageReceived);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &Application::onBrokerConnectedChanged);
	//connect(m_revitest, &RevitestDevice::sendRpcMessage, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::sendMessage);
	//connect(m_revitest->shvTree()->root(), &shv::iotqt::node::ShvRootNode::sendRpcMessage, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::sendMessage);

	m_rpcConnection->open();
}

Application::~Application()
{
}

Application *Application::instance()
{
	return qobject_cast<Application *>(QCoreApplication::instance());
}

void Application::onBrokerConnectedChanged(bool is_connected)
{
	if(is_connected) {
		std::string cpon;
	}
}


