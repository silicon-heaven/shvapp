#include "websocketserverconnection.h"
#include "websocket.h"

#include <shv/core/utils.h>
#include <shv/coreqt/log.h>

#include <QWebSocket>

namespace rpc {

WebSocketServerConnection::WebSocketServerConnection(QWebSocket* socket, QObject *parent)
	: Super(new WebSocket(socket), parent)
{
}

} // namespace rpc
