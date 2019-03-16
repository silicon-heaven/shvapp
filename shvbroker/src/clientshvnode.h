#pragma once

#include <shv/iotqt/node/shvnode.h>

namespace rpc { class TcpServer; class ClientBrokerConnection; }

class ClientShvNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT
private:
	using Super = shv::iotqt::node::ShvNode;
public:
	ClientShvNode(rpc::ClientBrokerConnection *conn, shv::iotqt::node::ShvNode *parent = nullptr);
	~ClientShvNode() override;

	rpc::ClientBrokerConnection * connection() const {return m_connections.value(0);}
	QList<rpc::ClientBrokerConnection *> connections() const {return m_connections;}

	void addConnection(rpc::ClientBrokerConnection *conn);
	void removeConnection(rpc::ClientBrokerConnection *conn);

	void handleRawRpcRequest(shv::chainpack::RpcValue::MetaData &&meta, std::string &&data) override;
	shv::chainpack::RpcValue hasChildren(const StringViewList &shv_path) override;
private:
	QList<rpc::ClientBrokerConnection *> m_connections;
};

class MasterBrokerShvNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT
private:
	using Super = shv::iotqt::node::ShvNode;
public:
	MasterBrokerShvNode(shv::iotqt::node::ShvNode *parent = nullptr);
	~MasterBrokerShvNode() override;
};

