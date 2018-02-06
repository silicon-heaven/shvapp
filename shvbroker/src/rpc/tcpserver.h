#pragma once

#include <shv/core/utils.h>
#include <shv/chainpack/rpcvalue.h>

#include <QTcpServer>

namespace shv { namespace chainpack { class RpcMessage; }}

namespace rpc {

class ServerConnection;

class TcpServer : public QTcpServer
{
	Q_OBJECT
	//Q_PROPERTY(int numConnections READ numConnections)

	using Super = QTcpServer;

	//SHV_FIELD_IMPL(int, p, P, ort)
public:
	explicit TcpServer(QObject *parent = 0);

	bool start(int port);
	std::vector<unsigned> connectionIds() const;
	ServerConnection* connectionById(unsigned connection_id);
protected:
	void onNewConnection();
	void onConnectionDeleted(int connection_id);
protected:
	std::map<unsigned, ServerConnection*> m_connections;
};

} // namespace rpc

