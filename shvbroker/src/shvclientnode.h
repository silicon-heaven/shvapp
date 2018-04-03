#pragma once

#include <shv/iotqt/node/shvnode.h>

namespace rpc { class TcpServer; class ServerConnection; }

class ShvClientNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT
private:
	using Super = shv::iotqt::node::ShvNode;
public:
	ShvClientNode(rpc::ServerConnection *connection, shv::iotqt::node::ShvNode *parent = nullptr);
	~ShvClientNode() override;

	rpc::ServerConnection * connection() const {return m_connection;}

	void processRawData(const shv::chainpack::RpcValue::MetaData &meta, std::string &&data) override;
private:
	rpc::ServerConnection * m_connection = nullptr;
};

