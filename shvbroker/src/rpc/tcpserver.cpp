#include "serverconnection.h"
#include "tcpserver.h"

#include <shv/coreqt/log.h>

#include <QTcpSocket>

namespace rpc {

TcpServer::TcpServer(QObject *parent)
	: Super(parent)
{
	connect(this, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);
}

bool TcpServer::start(int port)
{
	shvInfo() << "Starting RPC server on port:" << port;
	if (!listen(QHostAddress::AnyIPv4, port)) {
		shvError() << tr("Unable to start the server: %1.").arg(errorString());
		close();
		return false;
	}
	shvInfo().nospace() << "RPC server is listenning on " << serverAddress().toString() << ":" << serverPort();
	return true;
}

ServerConnection *TcpServer::connectionById(int connection_id)
{
	auto it = m_connections.find(connection_id);
	if(it == m_connections.end())
		return nullptr;
	return it->second;
}
/*
int TcpServer::numConnections()
{
	QList<ServerConnection*> cns = findChildren<ServerConnection*>(QString(), Qt::FindDirectChildrenOnly);
	return cns.count();
}
*/
void TcpServer::onNewConnection()
{
	QTcpSocket *sock = nextPendingConnection();
	if(sock) {
		shvInfo().nospace() << "agent connected: " << sock->peerAddress().toString() << ':' << sock->peerPort();
		ServerConnection *c = new ServerConnection(sock, this);
		m_connections[c->connectionId()] = c;
		int cid = c->connectionId();
		connect(c, &ServerConnection::destroyed, [this, cid]() {
			onConnectionDeleted(cid);
		});
		connect(c, &ServerConnection::rpcMessageReceived, this, &TcpServer::rpcMessageReceived);
	}
}

void TcpServer::onConnectionDeleted(int connection_id)
{
	m_connections.erase(connection_id);
}

} // namespace rpc
