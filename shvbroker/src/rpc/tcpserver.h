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
	//int numConnections();
	ServerConnection* connectionById(int connection_id);
	//Q_SIGNAL void rpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	Q_SIGNAL void rpcDataReceived(const shv::chainpack::RpcValue::MetaData &meta, const std::string &data);
protected:
	void onNewConnection();
	void onConnectionDeleted(int connection_id);
protected:
	std::map<int, ServerConnection*> m_connections;
};

} // namespace rpc

