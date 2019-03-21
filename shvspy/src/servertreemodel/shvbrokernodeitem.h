#pragma once

#include "shvnodeitem.h"

//#include <shv/chainpack/rpcvalue.h>

#include <map>

namespace shv { namespace chainpack { class RpcValue; class RpcMessage; } }
namespace shv { namespace iotqt { namespace rpc { class ClientConnection; } } }

class ShvBrokerNodeItem : public ShvNodeItem
{
	Q_OBJECT
private:
	using Super = ShvNodeItem;
public:
	static const QString SUBSCRIPTIONS;
	enum class OpenStatus {Invalid = 0, Disconnected, Connecting, Connected};
	enum SubscriptionItem {Path = 0, Method, IsPermanent, IsSubscribedAfterConnect, IsEnabled, Count};
	Q_ENUM(SubscriptionItem)
public:
	explicit ShvBrokerNodeItem(ServerTreeModel *m, const std::string &server_name);
	~ShvBrokerNodeItem() Q_DECL_OVERRIDE;

	QVariant data(int role = Qt::UserRole + 1) const Q_DECL_OVERRIDE;

	OpenStatus openStatus() const {return m_openStatus;}

	bool isOpen() {return openStatus() == OpenStatus::Connected;}
	void open();
	void close();
	//QString connectionErrorString();

	QVariantMap serverProperties() const;
	void setServerProperties(const QVariantMap &props);

	void setSubscriptionList(const QVariantList &subs);
	void addSubscription(const std::string &shv_path, const std::string &method);
	void enableSubscription(const std::string &shv_path, const std::string &method, bool is_enabled);

	shv::iotqt::rpc::ClientConnection *clientConnection();

	int callNodeRpcMethod(const std::string &calling_node_shv_path, const std::string &method, const shv::chainpack::RpcValue &params);

	ShvNodeItem *findNode(const std::string &path, std::string *path_rest = nullptr);

	int brokerId() const { return m_brokerId; }

	Q_SIGNAL void subscriptionAdded(const std::string &path, const std::string &method);
	Q_SIGNAL void brokerConnectedChange(bool is_connected);

private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void createSubscriptions();
	int callSubscribe(const std::string &shv_path, std::string method);
	int callUnsubscribe(const std::string &shv_path, std::string method);

private:
	int m_brokerId;
	QVariantMap m_serverPropeties;
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
	OpenStatus m_openStatus = OpenStatus::Disconnected;
	struct RpcRequestInfo;
	std::map<int, RpcRequestInfo> m_runningRpcRequests;
};

