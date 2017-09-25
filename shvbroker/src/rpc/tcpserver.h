#pragma once

#include <shv/core/utils.h>

#include <QTcpServer>

namespace rpc {

class TcpServer : public QTcpServer
{
	Q_OBJECT
	//Q_PROPERTY(int numConnections READ numConnections)

	using Super = QTcpServer;

	//SHV_FIELD_IMPL(int, p, P, ort)
public:
	explicit TcpServer(QObject *parent = 0);

	bool start(int port);
	int numConnections();
protected:
	void onNewConnection();
};

} // namespace rpc

