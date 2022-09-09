#include <iostream>
#include "historyapp.h"
#include "appclioptions.h"
#include "src/shvjournalnode.h"
#include "src/leafnode.h"

#include <shv/iotqt/rpc/rpccall.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/node/localfsnode.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/tunnelctl.h>
#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/cponreader.h>

#include <shv/core/stringview.h>

#include <QDirIterator>
#include <QTimer>

#include <QCoroSignal>

using namespace std;
namespace cp = shv::chainpack;
namespace si = shv::iotqt;

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_APP_NAME, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_DEVICE_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_DEVICE_TYPE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
};

size_t AppRootNode::methodCount(const StringViewList& shv_path)
{
	if (shv_path.empty()) {
		return meta_methods.size();
	}
	return 0;
}

const cp::MetaMethod* AppRootNode::metaMethod(const StringViewList& shv_path, size_t ix)
{
	if (shv_path.empty()) {
		if (meta_methods.size() <= ix) {
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()));
		}
		return &(meta_methods[ix]);
	}
	return nullptr;
}

cp::RpcValue AppRootNode::callMethod(const StringViewList& shv_path, const std::string& method, const cp::RpcValue& params, const cp::RpcValue& user_id)
{
	if (shv_path.empty()) {
		if (method == cp::Rpc::METH_APP_NAME) {
			return QCoreApplication::instance()->applicationName().toStdString();
		}
	}
	return Super::callMethod(shv_path, method, params, user_id);
}

cp::RpcValue AppRootNode::callMethodRq(const cp::RpcRequest& rq)
{
	if (rq.shvPath().asString().empty()) {
		if (rq.method() == cp::Rpc::METH_DEVICE_ID) {
			HistoryApp* app = HistoryApp::instance();
			const cp::RpcValue::Map& opts = app->rpcConnection()->connectionOptions().toMap();
			const cp::RpcValue::Map& dev = opts.value(cp::Rpc::KEY_DEVICE).toMap();
			return dev.value(cp::Rpc::KEY_DEVICE_ID).toString();
		}

		if (rq.method() == cp::Rpc::METH_DEVICE_TYPE) {
			return "HistoryProvider";
		}
	}
	return Super::callMethodRq(rq);
}


namespace {
enum class SlaveFound {
	Yes,
	No
};

void createTree(shv::iotqt::node::ShvNode* parent_node, const cp::RpcValue::Map& tree, const QString& node_name, std::vector<SlaveHpInfo>& slave_hps, SlaveFound slave_found)
{
	shv::iotqt::node::ShvNode* node;
	if (auto meta_node = tree.value("_meta").asMap(); meta_node.hasKey("HP") || meta_node.hasKey("HP3")) {
		auto hp_node = meta_node.value("HP", meta_node.value("HP3")).asMap();
		auto log_type = hp_node.value("pushLog").toBool() ? LogType::PushLog : LogType::Normal;
		node = new LeafNode(node_name.toStdString(), log_type, parent_node);
		if (!parent_node->shvPath().empty() && slave_found != SlaveFound::Yes) {
			slave_found = SlaveFound::Yes;
			slave_hps.push_back(SlaveHpInfo {
				.is_leaf = meta_node.hasKey("HP") || !meta_node.value("HP3").asMap().value("slave").toBool(),
				.log_type = log_type,
				.shv_path = node->shvPath(),
			});
		}
	} else {
		node = new shv::iotqt::node::ShvNode(node_name.toStdString(), parent_node);
	}

	for (const auto& [k, v] : tree) {
		if (k == "_meta") {
			continue;
		}

		if (v.type() == cp::RpcValue::Type::Map) {
			createTree(node, v.asMap(), QString::fromStdString(k), slave_hps, slave_found);
		}
	}
}

auto parse_size_option(const std::string& size_option, bool* success)
{
	*success = true;

	QString limit = QString::fromStdString(size_option);
	char suffix = 0;
	if (limit.endsWith('k') || limit.endsWith('M') || limit.endsWith('G')) {
		suffix = limit[limit.length() - 1].toLatin1();
		limit = limit.left(limit.length() - 1);
	}

	auto cache_size_limit_bytes = limit.toLongLong(success);
	if (!success) {
		shvError() << "Invalid cacheSizeLimit" << size_option;
		return 0LL;
	}

	if (suffix) {
		switch (suffix) {
		case 'k':
			cache_size_limit_bytes *= 1024;
			break;
		case 'M':
			cache_size_limit_bytes *= 1024;
			cache_size_limit_bytes *= 1024;
			break;
		case 'G':
			cache_size_limit_bytes *= 1024;
			cache_size_limit_bytes *= 1024;
			cache_size_limit_bytes *= 1024;
			break;
		default:
			*success = false;
			shvError() << "Invalid cacheSizeLimit suffix" << suffix;
			break;
		}
	}

	return cache_size_limit_bytes;
}
}

void HistoryApp::sanitizeNext()
{
	if (!m_sanitizerIterator.hasNext()) {
		m_sanitizerIterator = QListIterator(m_journalNodes);
	}

	// m_journalNodes is never empty, because we don't run the sanitizer when there are no journal nodes
	auto node = m_sanitizerIterator.next();
	node->sanitizeSize();
}

HistoryApp::HistoryApp(int& argc, char** argv, AppCliOptions* cli_opts, shv::iotqt::rpc::DeviceConnection* rpc_connection)
	: Super(argc, argv)
	  , m_rpcConnection(rpc_connection)
	  , m_cliOptions(cli_opts)
	  , m_sanitizerIterator(m_journalNodes) // I have to initialize this one, because it doesn't have a default ctor
{
	m_rpcConnection->setParent(this);

	if (!cli_opts->user_isset()) {
		cli_opts->setUser("iot");
	}

	if (!cli_opts->password_isset()) {
		cli_opts->setPassword("lub42DUB");
	}

	bool success;
	auto size_limit_bytes = parse_size_option(cli_opts->journalCacheSizeLimit(), &success);

	if (!success) {
		auto default_limit_bytes = cli_opts->option("app.cacheSizeLimit").defaultValue().asString();
		shvError() << "Invalid cacheSizeLimit:" << cli_opts->journalCacheSizeLimit() << "using the default:" << default_limit_bytes;
		size_limit_bytes = parse_size_option(default_limit_bytes, &success);
	}

	shvInfo() << "Cache size limit:" << size_limit_bytes << "bytes";
	shvInfo() << "Sanitizer interval:" << cli_opts->journalSanitizerInterval() << "seconds";
	shvInfo() << "Log max age:" << cli_opts->logMaxAge() << "seconds";

	m_rpcConnection->setCliOptions(cli_opts);
	m_totalCacheSizeLimit = size_limit_bytes;

	connect(m_rpcConnection, &si::rpc::ClientConnection::brokerConnectedChanged, this, &HistoryApp::onBrokerConnectedChanged);
	connect(m_rpcConnection, &si::rpc::ClientConnection::rpcMessageReceived, this, &HistoryApp::onRpcMessageReceived);

	m_root = new AppRootNode();
	m_shvTree = new si::node::ShvNodeTree(m_root, this);
	connect(m_shvTree->root(), &si::node::ShvRootNode::sendRpcMessage, m_rpcConnection, &si::rpc::ClientConnection::sendMessage);

	QTimer::singleShot(0, m_rpcConnection, &si::rpc::ClientConnection::open);
}

HistoryApp::~HistoryApp()
{
	shvInfo() << "destroying history provider application";
}

HistoryApp* HistoryApp::instance()
{
	return qobject_cast<HistoryApp*>(QCoreApplication::instance());
}

QCoro::Task<void, QCoro::TaskOptions<QCoro::Options::AbortOnException>> HistoryApp::onBrokerConnectedChanged(bool is_connected)
{
	m_isBrokerConnected = is_connected;
	auto call = shv::iotqt::rpc::RpcCall::create(HistoryApp::instance()->rpcConnection())
		->setShvPath("sites")
		->setMethod("getSites");
	call->start();
	auto [result, error] = co_await qCoro(call, &shv::iotqt::rpc::RpcCall::maybeResult);

	if (!error.isEmpty()) {
		shvError() << "Couldn't retrieve sites:" << error;
		co_return;
	}

	std::vector<SlaveHpInfo> slave_hps;
	createTree(m_root, result.asMap(), "shv", slave_hps, SlaveFound::No);

	m_shvJournalNode = new ShvJournalNode(slave_hps, m_root);

	m_journalNodes = m_shvTree->findChildren<ShvJournalNode*>();
	if (m_journalNodes.size() != 0) {
		m_singleCacheSizeLimit = m_totalCacheSizeLimit / m_journalNodes.size();

		m_sanitizerIterator = QListIterator(m_journalNodes);
		auto timer = new QTimer(this);
		connect(timer, &QTimer::timeout, this, &HistoryApp::sanitizeNext);
		timer->start(m_cliOptions->journalSanitizerInterval() * 1000);
	}
}

void HistoryApp::onRpcMessageReceived(const cp::RpcMessage& msg)
{
	shvLogFuncFrame() << msg.toCpon();
	if (msg.isRequest()) {
		cp::RpcRequest rq(msg);
		shvDebug() << "RPC request received:" << rq.toPrettyString();
		if (m_shvTree->root()) {
			m_shvTree->root()->handleRpcRequest(rq);
		}
	} else if (msg.isResponse()) {
		cp::RpcResponse rp(msg);
		shvDebug() << "RPC response received:" << rp.toPrettyString();
	} else if (msg.isSignal()) {
		cp::RpcSignal nt(msg);
		shvDebug() << "RPC notify received:" << nt.toPrettyString();
	}
}
