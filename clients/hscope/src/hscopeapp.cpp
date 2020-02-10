#include "hscopeapp.h"
#include "appclioptions.h"
#include "hnodebrokers.h"
#include "hnodetest.h"
#include "telegram.h"

#include <shv/iotqt/node/shvnode.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/core/utils/shvfilejournal.h>
#include <shv/core/utils/shvlogtypeinfo.h>
#include <shv/core/utils/shvpath.h>
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
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_DEVICE_TYPE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_CLIENT_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_GET_LOG, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
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
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()))
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
		if(method == cp::Rpc::METH_DEVICE_TYPE) {
			return "HolyScope";
		}
		if(method == cp::Rpc::METH_CLIENT_ID) {
			return HScopeApp::instance()->rpcConnection()->loginResult().value(cp::Rpc::KEY_CLIENT_ID);
		}
		if(method == cp::Rpc::METH_GET_LOG) {
			return HScopeApp::instance()->shvJournal()->getLog(shv::core::utils::ShvGetLogParams(params));
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
	m_shvJournal = new shv::core::utils::ShvFileJournal(applicationName().toStdString(), [this](std::vector<shv::core::utils::ShvJournalEntry> &s) { this->getSnapshot(s); });
	if(cli_opts->shvJournalDir_isset())
		m_shvJournal->setJournalDir(cli_opts->shvJournalDir());
	m_shvJournal->setFileSizeLimit(cli_opts->shvJournalFileSizeLimit());
	m_shvJournal->setJournalSizeLimit(cli_opts->shvJournalSizeLimit());
	shv::core::utils::ShvLogTypeInfo type_info
	{
		// types
		{
			{
				"Status",
				{
					shv::core::utils::ShvLogTypeDescr::Type::Map,
					{
						{"val", "Enum", shv::chainpack::RpcValue::Map{{"Unknown", -1}, {"OK", 0}, {"Warning", 1}, {"Error", 2}}},
						{"msg", "String"},
					},
				}
			},
		},
		// paths
		{
			{ "status", {"Status"} },
		},
	};
	m_shvJournal->setTypeInfo(type_info);

	m_rpcConnection = new shv::iotqt::rpc::DeviceConnection(this);

	m_rpcConnection->setCliOptions(cli_opts);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &HScopeApp::brokerConnectedChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &HScopeApp::rpcMessageReceived);

	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::brokerConnectedChanged, this, &HScopeApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &HScopeApp::onRpcMessageReceived);

	AppRootNode *root = new AppRootNode();
	m_shvTree = new si::node::ShvNodeTree(root, this);
	connect(m_shvTree->root(), &si::node::ShvRootNode::sendRpcMessage, m_rpcConnection, &si::rpc::ClientConnection::sendMessage);
}

HScopeApp::~HScopeApp()
{
	shvInfo() << "destroying hscope application";
}

HScopeApp *HScopeApp::instance()
{
	return qobject_cast<HScopeApp *>(QCoreApplication::instance());
}

void HScopeApp::onHNodeStatusChanged(const std::string &shv_path, const NodeStatus &status)
{
	shvJournal()->append({shv_path, status.toRpcValue()});
	shv::iotqt::rpc::DeviceConnection *conn = rpcConnection();
	if(conn->isBrokerConnected()) {
		// log changes
		conn->sendShvNotify(shv_path, "statusChanged", status.toRpcValue());
	}
	emit alertStatusChanged(shv_path, status);
}

void HScopeApp::onHNodeOverallStatusChanged(const std::string &shv_path, const NodeStatus &status)
{
	shv::iotqt::rpc::DeviceConnection *conn = rpcConnection();
	if(conn->isBrokerConnected()) {
		// log changes
		conn->sendShvNotify(shv_path, "overallStatusChanged", status.toRpcValue());
	}
}

void HScopeApp::start()
{
	createNodes();
	//shvInfo() << "Telegram plugin enabled:" << cliOptions()->isTelegramEnabled();
	if(cliOptions()->isTelegramEnabled()) {
		m_telegram = new Telegram(nullptr);
		m_shvTree->mount("plugins/telegram", m_telegram);
		m_telegram->start();
	}
	QTimer::singleShot(0, m_rpcConnection, &shv::iotqt::rpc::ClientConnection::open);
}

NodeStatus HScopeApp::overallNodesStatus()
{
	return m_brokersNode->overallStatus();
}

void HScopeApp::createNodes()
{
	if(m_brokersNode)
		delete m_brokersNode;
	m_brokersNode = new HNodeBrokers("brokers", nullptr);
	m_brokersNode->setParent(m_shvTree->root());
	m_brokersNode->load();
}

void HScopeApp::getSnapshot(std::vector<shv::core::utils::ShvJournalEntry> &snapshot)
{
	auto lst = m_shvTree->root()->findChildren<HNodeTest*>(QString(), Qt::FindChildrenRecursively);
	for(const HNodeTest *nd : lst) {
		shv::core::utils::ShvJournalEntry e;
		e.path = nd->shvPath() + "/status";
		e.value = nd->status().toRpcValue();
		snapshot.push_back(std::move(e));
	}
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
		m_shvTree->root()->handleRpcRequest(rq);
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

