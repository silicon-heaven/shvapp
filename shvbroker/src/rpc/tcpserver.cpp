#include "tcpserver.h"
#include "serverconnection.h"

namespace rpc {

TcpServer::TcpServer(QObject *parent)
	: Super(parent)
{

}

ServerConnection *TcpServer::connectionById(unsigned connection_id)
{
	return qobject_cast<rpc::ServerConnection *>(Super::connectionById(connection_id));
}

shv::iotqt::server::ServerConnection *TcpServer::createServerConnection(QTcpSocket *socket, QObject *parent)
{
	return new ServerConnection(socket, parent);
}

} // namespace rpc
