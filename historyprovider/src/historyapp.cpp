#include <iostream>
#include "historyapp.h"
#include "appclioptions.h"
#include "src/shvjournalnode.h"

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

shv::iotqt::node::ShvNode* createTree(const cp::RpcValue::Map& tree, const std::string& parent_name, std::string remote_log_shv_path, const std::string& node_name, SlaveFound slave_found)
{
	using shv::core::Utils;
	if (node_name == "_meta" && tree.hasKey("HP")) {
		std::string log_shv_path_suffix;
		auto log_type = LogType::Normal;
		if (tree.value("HP").asMap().value("legacy").toBool()) {
			log_type = LogType::Legacy;
		}

		if (tree.value("HP").asMap().value("pushLog").toBool()) {
			log_type = LogType::PushLog;
		}

		if (log_type != LogType::Legacy) {
			remote_log_shv_path = Utils::joinPath(remote_log_shv_path, slave_found == SlaveFound::No ? ".app/shvjournal"s : "shvjournal"s);
		}

		return new ShvJournalNode(QString::fromStdString(parent_name), QString::fromStdString(remote_log_shv_path), log_type);
	}

	shv::iotqt::node::ShvNode* res = nullptr;
	auto new_parent_name = Utils::joinPath(parent_name, node_name);
	auto new_remote_log_shv_path = Utils::joinPath(remote_log_shv_path, node_name);
	// This block looks for a slave HP provider. It'll be used for all log fetching of this particular shv node. We
	// will still recurse until we find a "HP" key, because we can't easily know what kind of nodes the slave provider
	// contains.
	//
	// We'll also skip the first one (when parent_name is empty), because that one refers to this HP instance.
	if (!parent_name.empty() && slave_found == SlaveFound::No && tree.value("_meta").asMap().hasKey("HP3")) {
		new_remote_log_shv_path = Utils::joinPath(new_remote_log_shv_path, ".local/history/shv"s);
		slave_found = SlaveFound::Yes;
	}

	for (const auto& [k, v] : tree) {
		if (v.type() == cp::RpcValue::Type::Map) {
			auto node = createTree(v.asMap(), new_parent_name, new_remote_log_shv_path, k, slave_found);
			if (node) {
				if (!res) {
					res = new shv::iotqt::node::ShvNode(node_name);
				}

				node->setParentNode(res);
			}
		}
	}

	return res;
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

HistoryApp::HistoryApp(int& argc, char** argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	  , m_cliOptions(cli_opts)
	  , m_sanitizerIterator(m_journalNodes) // I have to initialize this one, because it doesn't have a default ctor
{
	m_rpcConnection = new si::rpc::DeviceConnection(this);

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

QCoro::Task<> HistoryApp::onBrokerConnectedChanged(bool is_connected)
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

	auto nodes = createTree(result.asMap(), "", "", "shv", SlaveFound::No);
	if (nodes) {
		nodes->setParentNode(m_root);
	}

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
