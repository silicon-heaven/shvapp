#pragma once

#include "hnode.h"

#include <shv/iotqt/node/shvnode.h>
#include <shv/coreqt/utils.h>

#include <QCoreApplication>
#include <QDateTime>

class AppCliOptions;
class QFileSystemWatcher;

class Telegram;

namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}
namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}
namespace shv { namespace core { namespace utils { class ShvFileJournal; struct ShvJournalEntry; }}}

class HNodeBrokers;

class AppRootNode : public shv::iotqt::node::ShvRootNode
{
	using Super = shv::iotqt::node::ShvRootNode;
public:
	explicit AppRootNode(QObject *parent = nullptr) : Super(parent) {}

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;
};

class HScopeApp : public QCoreApplication
{
	Q_OBJECT
private:
	using Super = QCoreApplication;

public:
	HScopeApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~HScopeApp() Q_DECL_OVERRIDE;

	static HScopeApp *instance();
	shv::iotqt::rpc::DeviceConnection *rpcConnection() const {return m_rpcConnection;}
	AppCliOptions* cliOptions() {return m_cliOptions;}
	shv::core::utils::ShvFileJournal *shvJournal() {return m_shvJournal;}

	void start();

	shv::iotqt::node::ShvNodeTree *shvTree() { return m_shvTree; }
	NodeStatus::Level overallNodesStatus();

	Q_SIGNAL void rpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	Q_SIGNAL void brokerConnectedChanged(bool is_connected);
	Q_SIGNAL void alertStatusChanged(const std::string &shv_path, const NodeStatus &status);

	void onHNodeStatusChanged(const std::string &shv_path, const NodeStatus &status);
	void onHNodeOverallStatusChanged(const std::string &shv_path, NodeStatus::Level level);
private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void createNodes();
	void getSnapshot(std::vector<shv::core::utils::ShvJournalEntry> &snapshot);
private:
	shv::iotqt::rpc::DeviceConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;
	shv::iotqt::node::ShvNodeTree *m_shvTree = nullptr;
	HNodeBrokers *m_brokersNode = nullptr;
	shv::core::utils::ShvFileJournal *m_shvJournal = nullptr;
	Telegram *m_telegram = nullptr;
};

