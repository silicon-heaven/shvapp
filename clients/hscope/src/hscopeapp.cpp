#include "hscopeapp.h"
#include "appclioptions.h"
#include "hbrokersnode.h"

#include <shv/iotqt/node/shvnode.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/coreqt/log.h>

#include <QTimer>

#include <fstream>

namespace cp = shv::chainpack;
namespace si = shv::iotqt;

//===================================================================
// AppRootNode
//===================================================================
//static const char M_RUN_SCRIPT[] = "runScript";

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_BROWSE},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::GRANT_BROWSE},
	{cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::GRANT_BROWSE},
	{cp::Rpc::METH_CLIENT_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::GRANT_READ},
};

size_t AppRootNode::methodCount(const StringViewList &shv_path)
{
	if(shv_path.empty())
		return meta_methods.size();
	return 0;
}

const shv::chainpack::MetaMethod *AppRootNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty()) {
		if(meta_methods.size() <= ix)
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()));
		return &(meta_methods[ix]);
	}
	return nullptr;
}

shv::chainpack::RpcValue AppRootNode::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == cp::Rpc::METH_APP_NAME) {
			return QCoreApplication::instance()->applicationName().toStdString();
		}
		if(method == cp::Rpc::METH_DEVICE_ID) {
			return HScopeApp::instance()->cliOptions()->deviceId();
		}
		if(method == cp::Rpc::METH_CLIENT_ID) {
			return HScopeApp::instance()->rpcConnection()->loginResult().value(cp::Rpc::KEY_CLIENT_ID);
		}
	}
	return Super::callMethod(shv_path, method, params);
}

//===================================================================
// HScopeApp
//===================================================================
HScopeApp::HScopeApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);

	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &HScopeApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &HScopeApp::onRpcMessageReceived);

	AppRootNode *root = new AppRootNode();
	m_shvTree = new si::node::ShvNodeTree(root, this);
	connect(m_shvTree->root(), &si::node::ShvRootNode::sendRpcMesage, m_rpcConnection, &si::rpc::ClientConnection::sendMessage);

	loadConfig();

	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
}

HScopeApp::~HScopeApp()
{
	shvInfo() << "destroying shv agent application";
}

HScopeApp *HScopeApp::instance()
{
	return qobject_cast<HScopeApp *>(QCoreApplication::instance());
}

void HScopeApp::loadConfig()
{
	createNodes();
}

void HScopeApp::createNodes()
{
	if(m_brokersNode)
		delete m_brokersNode;
	m_brokersNode = new HBrokersNode("brokers", m_shvTree->root());
	m_brokersNode->load();
}
/*
const std::string &HScopeApp::logFilePath()
{
	static std::string log_file_path = QDir::tempPath().toStdString() + "/" + applicationName().toStdString() + ".log";
	return log_file_path;
}
*/
void HScopeApp::onBrokerConnectedChanged(bool is_connected)
{
	if(is_connected) {
	}
	else {
	}
}

void HScopeApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		cp::RpcResponse resp = cp::RpcResponse::forRequest(rq.metaData());
		try {
			m_shvTree->root()->handleRpcRequest(rq);
		}
		catch (shv::core::Exception &e) {
			resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, e.message()));
		}
		if(resp.requestId().toInt() > 0) // RPC calls with requestID == 0 does not expect response
			m_rpcConnection->sendMessage(resp);
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rsp(msg);
		if(rsp.isError())
			shvError() << "RPC error response received:" << rsp.toCpon();

	}
	else if(msg.isSignal()) {
		cp::RpcSignal ntf(msg);
		shvDebug() << "RPC notify received:" << ntf.toCpon();
	}
}

