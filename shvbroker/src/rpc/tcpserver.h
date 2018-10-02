#pragma once

#include <shv/iotqt/rpc/tcpserver.h>

namespace rpc {

class ServerConnection;

class TcpServer : public shv::iotqt::rpc::TcpServer
{
	Q_OBJECT
	using Super = shv::iotqt::rpc::TcpServer;
public:
	TcpServer(QObject *parent = 0);

	ServerConnection* connectionById(int connection_id);
protected:
	shv::iotqt::rpc::ServerConnection* createServerConnection(QTcpSocket *socket, QObject *parent) override;
};

} // namespace rpc
