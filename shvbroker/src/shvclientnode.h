#pragma once

#include <shv/iotqt/shvnode.h>

namespace rpc { class TcpServer; class ServerConnection; }

class ShvClientNode : public shv::iotqt::ShvNode
{
	Q_OBJECT
private:
	using Super = shv::iotqt::ShvNode;
public:
	ShvClientNode(rpc::ServerConnection *connection, QObject *parent = nullptr);
	~ShvClientNode() override;

	rpc::ServerConnection * connection() const {return m_connection;}
private:
	rpc::ServerConnection * m_connection = nullptr;
};

