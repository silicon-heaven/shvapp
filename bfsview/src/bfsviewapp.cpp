#include "bfsviewapp.h"
#include "appclioptions.h"
#include "sessionprocess.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/tunnelconnection.h>
#include <shv/iotqt/rpc/tunnelhandle.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/node/localfsnode.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/metamethod.h>

#include <shv/core/stringview.h>

#include <QProcess>
#include <QTimer>

namespace cp = shv::chainpack;

static const char PWR_STATUS[] = "pwrStatus";

static const char METH_SIM_SET[] = "sim_set";

static std::vector<cp::MetaMethod> meta_methods_root {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, false},
	{cp::Rpc::METH_CONNECTION_TYPE, cp::MetaMethod::Signature::RetVoid, false},
};

size_t AppRootNode::methodCount()
{
	return meta_methods_root.size();
}

const shv::chainpack::MetaMethod *AppRootNode::metaMethod(size_t ix)
{
	if(meta_methods_root.size() <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods_root.size()));
	return &(meta_methods_root[ix]);
}

shv::chainpack::RpcValue AppRootNode::call(const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(method == cp::Rpc::METH_APP_NAME) {
		return QCoreApplication::instance()->applicationName().toStdString();
	}
	if(method == cp::Rpc::METH_CONNECTION_TYPE) {
		return BfsViewApp::instance()->rpcConnection()->connectionType();
	}
	return Super::call(method, params);
}
/*
shv::chainpack::RpcValue AppRootNode::processRpcRequest(const shv::chainpack::RpcRequest &rq)
{
	if(rq.method() == METH_OPEN_RSH) {
		BfsViewApp *app = BfsViewApp::instance();
		app->openRsh(rq);
		return cp::RpcValue();
	}
	return Super::processRpcRequest(rq);
}
*/
static std::vector<cp::MetaMethod> meta_methods_pwrstatus {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, false},
	{cp::Rpc::NTF_VAL_CHANGED, cp::MetaMethod::Signature::VoidParam, true},
	{METH_SIM_SET, cp::MetaMethod::Signature::VoidParam, false},
};

size_t PwrStatusNode::methodCount()
{
	return meta_methods_pwrstatus.size();
}

const shv::chainpack::MetaMethod *PwrStatusNode::metaMethod(size_t ix)
{
	if(meta_methods_pwrstatus.size() <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods_pwrstatus.size()));
	return &(meta_methods_pwrstatus[ix]);
}

shv::chainpack::RpcValue PwrStatusNode::call(const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(method == cp::Rpc::METH_GET) {
		return pwrStatus();
	}
	if(method == METH_SIM_SET) {
		unsigned s = params.toUInt();
		setPwrStatus(s);
		return true;
	}
	return Super::call(method, params);
}

void PwrStatusNode::setPwrStatus(unsigned s)
{
	if(s == m_pwrStatus)
		return;
	m_pwrStatus = s;
	emitPwrStatusChanged();
}

void PwrStatusNode::emitPwrStatusChanged()
{
	cp::RpcNotify ntf;
	ntf.setMethod(cp::Rpc::NTF_VAL_CHANGED);
	ntf.setParams(m_pwrStatus);
	ntf.setShvPath(PWR_STATUS);
	rootNode()->emitSendRpcMesage(ntf);
}

BfsViewApp::BfsViewApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);

	if(!cli_opts->user_isset())
		cli_opts->setUser("iot");
	if(!cli_opts->password_isset())
		cli_opts->setPassword("lub42DUB");
	if(!cli_opts->deviceId_isset())
		cli_opts->setDeviceId("bfsview_test");
	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &BfsViewApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &BfsViewApp::onRpcMessageReceived);

	AppRootNode *root = new AppRootNode();
	m_shvTree = new shv::iotqt::node::ShvNodeTree(root, this);
	m_pwrStatusNode = new PwrStatusNode();
	m_shvTree->mount(PWR_STATUS, m_pwrStatusNode);
	connect(m_shvTree->root(), &shv::iotqt::node::ShvRootNode::sendRpcMesage, m_rpcConnection, &shv::iotqt::rpc::DeviceConnection::sendMessage);

	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
	if(cli_opts->pwrStatusPublishInterval() > 0) {
		shvInfo() << "pwrStatus publis interval set to:" << cli_opts->pwrStatusPublishInterval() << "sec.";
		QTimer *tm = new QTimer(this);
		connect(tm, &QTimer::timeout, [this]() {
			m_pwrStatusNode->emitPwrStatusChanged();
		});
		tm->start(m_cliOptions->pwrStatusPublishInterval() * 1000);
	}
}

BfsViewApp::~BfsViewApp()
{
	shvInfo() << "destroying shv agent application";
}

BfsViewApp *BfsViewApp::instance()
{
	return qobject_cast<BfsViewApp *>(QCoreApplication::instance());
}

void BfsViewApp::onBrokerConnectedChanged(bool is_connected)
{
	if(!is_connected)
		return;
	try {

	}
	catch (shv::core::Exception &e) {
		shvError() << "Lublicator Testing FAILED:" << e.message();
	}
}

void BfsViewApp::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		cp::RpcResponse resp = cp::RpcResponse::forRequest(rq.metaData());
		try {
			//shvInfo() << "RPC request received:" << rq.toCpon();
			const cp::RpcValue shv_path = rq.shvPath();
			std::string path_rest;
			shv::iotqt::node::ShvNode *nd = m_shvTree->cd(shv_path.toString(), &path_rest);
			if(!nd)
				SHV_EXCEPTION("Path not found: " + shv_path.toString());
			rq.setShvPath(path_rest);
			shv::chainpack::RpcValue result = nd->processRpcRequest(rq);
			if(result.isValid())
				resp.setResult(result);
			else
				return;
		}
		catch (shv::core::Exception &e) {
			resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodInvocationException, e.message()));
		}
		if(resp.requestId().toInt() > 0) // RPC calls with requestID == 0 does not expect response
			m_rpcConnection->sendMessage(resp);
	}
	else if(msg.isResponse()) {
		cp::RpcResponse rp(msg);
		shvInfo() << "RPC response received:" << rp.toCpon();
	}
	else if(msg.isNotify()) {
		cp::RpcNotify nt(msg);
		shvInfo() << "RPC notify received:" << nt.toCpon();
		/*
		if(nt.method() == cp::Rpc::NTF_VAL_CHANGED) {
			if(nt.shvPath() == "/test/shv/lublicator2/status") {
				shvInfo() << lublicatorStatusToString(nt.params().toUInt());
			}
		}
		*/
	}
}
/*
void ShvAgentApp::onRootNodeSendRpcMesage(const shv::chainpack::RpcMessage &msg)
{
	if(msg.isResponse()) {
		cp::RpcResponse resp(msg);
		if(resp.requestId().toUInt() == 0) // RPC calls with requestID == 0 does not expect response
			return;
		m_rpcConnection->sendMessage(resp);
		return;
	}
	shvError() << "Send message not implemented.";// << msg.toCpon();
}
*/

