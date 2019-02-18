#pragma once

#include <shv/iotqt/node/shvnode.h>
#include "brclabfsnode.h"
#include "brclabusers.h"

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class AppCliOptions;
class QTimer;

namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}

class AppRootNode : public BrclabFsNode
{
	using Super = BrclabFsNode;
public:
	explicit AppRootNode(const QString &root_path, Super *parent = nullptr);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
	shv::chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq) override;
private:
	bool m_isRootNodeValid = false;
};

class ShvBrclabProviderApp : public QCoreApplication
{
	Q_OBJECT
private:
	using Super = QCoreApplication;
public:
	ShvBrclabProviderApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~ShvBrclabProviderApp() Q_DECL_OVERRIDE;

	static ShvBrclabProviderApp *instance();
	shv::iotqt::rpc::DeviceConnection *rpcConnection() const {return m_rpcConnection;}

	AppCliOptions* cliOptions() {return m_cliOptions;}
	std::string brclabUsersFileName();
	BrclabUsers *brclabUsers();
private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void onNetworkManagerFinished(QNetworkReply *reply);

	shv::iotqt::rpc::DeviceConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;
	AppRootNode *m_root = nullptr;
	BrclabUsers m_brclabUsers;
	bool m_isBrokerConnected = false;
};

