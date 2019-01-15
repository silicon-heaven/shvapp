#pragma once

#include <QWebSocketServer>

class QWebSocket;
class ServerConnection;

namespace rpc {

class ServerConnection;

class WebSocketServer : public QWebSocketServer
{
	Q_OBJECT
	using Super = QWebSocketServer;
public:
	WebSocketServer(QObject *parent = nullptr);
	~WebSocketServer() override;

	bool start(int port = 0);

	std::vector<int> connectionIds() const;
	ServerConnection* connectionById(int connection_id);
private:
	ServerConnection* createServerConnection(QWebSocket *socket, QObject *parent);
	void onNewConnection();
	void onConnectionDeleted(int connection_id);
private:
	std::map<int, ServerConnection*> m_connections;
};

} // namespace rpc
