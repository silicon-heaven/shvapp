#pragma once

#include <shv/iotqt/node/shvnode.h>

#include <QCoreApplication>

class AppCliOptions;
class QDir;
class QTimer;

namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}
namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}

struct lua_State;

class AppRootNode : public shv::iotqt::node::ShvRootNode
{
	using Super = shv::iotqt::node::ShvRootNode;
public:
	explicit AppRootNode(QObject* parent = nullptr) : Super(parent) {}

	size_t methodCount(const StringViewList& shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList& shv_path, size_t ix) override;

	shv::chainpack::RpcValue callMethodRq(const shv::chainpack::RpcRequest& rq) override;
	shv::chainpack::RpcValue callMethod(const StringViewList& shv_path, const std::string& method, const shv::chainpack::RpcValue& params, const shv::chainpack::RpcValue& user_id) override;
};

class HolyScopeApp : public QCoreApplication
{
	Q_OBJECT
private:
	using Super = QCoreApplication;
public:
	HolyScopeApp(int& argc, char** argv, AppCliOptions* cli_opts);
	~HolyScopeApp() Q_DECL_OVERRIDE;

	static HolyScopeApp* instance();
	shv::iotqt::rpc::DeviceConnection* rpcConnection() const {return m_rpcConnection;}

	AppCliOptions* cliOptions() {return m_cliOptions;}

	void subscribeLua(const std::string& path);
	int callShvMethod(const std::string& path, const std::string& method, const shv::chainpack::RpcValue& params = shv::chainpack::RpcValue());

private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage& msg);

	void evalLuaFile(const QString& fileName);
	void createPaths(const QDir& dir, shv::iotqt::node::ShvNode* node);

private:
	shv::iotqt::rpc::DeviceConnection* m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;

	shv::iotqt::node::ShvNodeTree* m_shvTree = nullptr;
	bool m_isBrokerConnected = false;

	unsigned m_currentTestIndex = 0;

	lua_State* m_state;
};

