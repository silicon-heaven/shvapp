#include "flatlineapp.h"
#include "appclioptions.h"

#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/core/log.h>
#include <shv/chainpack/rpcdriver.h>

namespace cp = shv::chainpack;
namespace iot = shv::iotqt;

const char *FlatLineApp::SIG_FASTCHNG = "fastchng";

FlatLineApp::FlatLineApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	//cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	m_rpcConnection = new shv::iotqt::rpc::ClientConnection(this);

	m_rpcConnection->setCliOptions(cli_opts);

	//connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, m_revitest, &RevitestDevice::onRpcMessageReceived);
	connect(m_rpcConnection, &iot::rpc::ClientConnection::brokerConnectedChanged, this, &FlatLineApp::onBrokerConnectedChanged);

	if(!cli_opts->deviceShvPath().empty())
		m_rpcConnection->open();
}

FlatLineApp::~FlatLineApp()
{
	shvInfo() << "destroying shv agent application";
}

FlatLineApp *FlatLineApp::instance()
{
	return qobject_cast<FlatLineApp *>(QCoreApplication::instance());
}

void FlatLineApp::onBrokerConnectedChanged(bool is_connected)
{
	if(is_connected) {
		std::string shv_path = cliOptions()->deviceShvPath();
		if(shv_path.empty()) {
			shvError() << "Anca SHV path not specified";
			return;
		}
		else {
			shvInfo() << "subscribing path:" << shv_path;
			rpcConnection()->callMethodSubscribe(shv_path, cp::Rpc::SIG_VAL_CHANGED);
		}
	}
}


