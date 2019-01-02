#pragma once

#include "commonrpcclienthandle.h"

#include <shv/iotqt/rpc/serverconnection.h>

#include <QObject>

class QWebSocket;

namespace rpc {

class WebSocketServerConnection : public ServerConnection
{
	Q_OBJECT

	using Super = shv::iotqt::rpc::ServerConnection;

public:
	WebSocketServerConnection(QWebSocket* socket, QObject *parent = nullptr);
	~WebSocketServerConnection() override;
};

} // namespace rpc
