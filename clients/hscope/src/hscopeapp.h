#pragma once

#include <shv/iotqt/node/shvnode.h>
#include <shv/coreqt/utils.h>

#include <QApplication>
#include <QDateTime>

class AppCliOptions;
class QFileSystemWatcher;

class NodeStatus;

namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}
namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}

class HNodeBrokers;

class AppRootNode : public shv::iotqt::node::ShvRootNode
{
	using Super = shv::iotqt::node::ShvRootNode;
public:
	explicit AppRootNode(QObject *parent = nullptr) : Super(parent) {}

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
};

class HScopeApp : public QApplication
{
	Q_OBJECT
private:
	using Super = QApplication;

public:
	HScopeApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~HScopeApp() Q_DECL_OVERRIDE;

	static HScopeApp *instance();
	shv::iotqt::rpc::DeviceConnection *rpcConnection() const {return m_rpcConnection;}
	AppCliOptions* cliOptions() {return m_cliOptions;}
	//static const std::string& logFilePath();
	void onHNodeStatusChanged(const std::string &shv_path, const NodeStatus &status);
private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void loadConfig();
	void createNodes();
private:
	shv::iotqt::rpc::DeviceConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;
	shv::iotqt::node::ShvNodeTree *m_shvTree = nullptr;
	HNodeBrokers *m_brokersNode = nullptr;


};

