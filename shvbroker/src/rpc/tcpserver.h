#pragma once

#include <shv/iotqt/server/tcpserver.h>

namespace rpc {

class ServerConnection;

class TcpServer : public shv::iotqt::server::TcpServer
{
	Q_OBJECT
	using Super = shv::iotqt::server::TcpServer;
public:
	TcpServer(QObject *parent = 0);

	ServerConnection* connectionById(unsigned connection_id);
protected:
	shv::iotqt::server::ServerConnection* createServerConnection(QTcpSocket *socket, QObject *parent) override;
};

} // namespace rpc
