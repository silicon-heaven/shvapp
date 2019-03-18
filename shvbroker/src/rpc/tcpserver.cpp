#include "tcpserver.h"
#include "clientbrokerconnection.h"

#include <shv/iotqt/rpc/socket.h>

namespace rpc {

TcpServer::TcpServer(QObject *parent)
	: Super(parent)
{

}

ClientBrokerConnection *TcpServer::connectionById(int connection_id)
{
	return qobject_cast<rpc::ClientBrokerConnection *>(Super::connectionById(connection_id));
}

shv::iotqt::rpc::ServerConnection *TcpServer::createServerConnection(QTcpSocket *socket, QObject *parent)
{
	return new ClientBrokerConnection(new shv::iotqt::rpc::TcpSocket(socket), parent);
}

} // namespace rpc
