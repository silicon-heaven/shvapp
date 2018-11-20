#pragma once

#include <shv/iotqt/node/shvnode.h>

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class AppCliOptions;
class QTimer;

namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}
namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}

class AppRootNode : public shv::iotqt::node::ShvRootNode
{
	using Super = shv::iotqt::node::ShvRootNode;
public:
	explicit AppRootNode(QObject *parent = nullptr) : Super(parent) {}

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;

	shv::chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq) override;
};

class ShvFileProviderApp : public QCoreApplication
{
	Q_OBJECT
private:
	using Super = QCoreApplication;
public:
	ShvFileProviderApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~ShvFileProviderApp() Q_DECL_OVERRIDE;

	static ShvFileProviderApp *instance();
	shv::iotqt::rpc::DeviceConnection *rpcConnection() const {return m_rpcConnection;}

	AppCliOptions* cliOptions() {return m_cliOptions;}
private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void onNetworkManagerFinished(QNetworkReply *reply);
	shv::iotqt::rpc::DeviceConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;

	shv::iotqt::node::ShvNodeTree *m_shvTree = nullptr;
	bool m_isBrokerConnected = false;
};

