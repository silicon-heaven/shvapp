#include "tcpserver.h"
#include "serverconnection.h"

namespace rpc {

TcpServer::TcpServer(QObject *parent)
	: Super(parent)
{

}

ServerConnection *TcpServer::connectionById(int connection_id)
{
	return qobject_cast<rpc::ServerConnection *>(Super::connectionById(connection_id));
}

shv::iotqt::rpc::ServerConnection *TcpServer::createServerConnection(QTcpSocket *socket, QObject *parent)
{
	return new ServerConnection(socket, parent);
}

} // namespace rpc
