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
	enum class OpenStatus {Invalid = 0, Disconnected, Connecting, Connected};
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

	void addSubscription(const std::string &shv_path, const std::string &method);

	shv::iotqt::rpc::ClientConnection *clientConnection();

	int callNodeRpcMethod(const std::string &calling_node_shv_path, const std::string &method, const shv::chainpack::RpcValue &params);

	ShvNodeItem *findNode(const std::string &path, std::string *path_rest = nullptr);

	Q_SIGNAL void subscriptionsCreated();
	Q_SIGNAL void subscriptionAdded(const std::string &path);

private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void createSubscriptions();
	int callCreateSubscription(const std::string &shv_path, std::string method);

private:
	QVariantMap m_serverPropeties;
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
	OpenStatus m_openStatus = OpenStatus::Disconnected;
	struct RpcRequestInfo;
	std::map<int, RpcRequestInfo> m_runningRpcRequests;

};

