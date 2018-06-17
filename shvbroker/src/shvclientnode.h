#pragma once

#include <shv/iotqt/node/shvnode.h>

namespace rpc { class TcpServer; class ServerConnection; }

class ShvClientNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT
private:
	using Super = shv::iotqt::node::ShvNode;
public:
	ShvClientNode(rpc::ServerConnection *conn, shv::iotqt::node::ShvNode *parent = nullptr);
	~ShvClientNode() override;

	rpc::ServerConnection * connection() const {return m_connections.value(0);}

	void addConnection(rpc::ServerConnection *conn);
	void removeConnection(rpc::ServerConnection *conn);

	void processRawData(const shv::chainpack::RpcValue::MetaData &meta, std::string &&data) override;
	shv::chainpack::RpcValue hasChildren() override;
private:
	QList<rpc::ServerConnection *> m_connections;
};

