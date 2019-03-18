#pragma once

#include <QWebSocketServer>

class QWebSocket;
class ServerConnection;

namespace rpc {

class ClientBrokerConnection;

class WebSocketServer : public QWebSocketServer
{
	Q_OBJECT
	using Super = QWebSocketServer;
public:
	WebSocketServer(QObject *parent = nullptr);
	~WebSocketServer() override;

	bool start(int port = 0);

	std::vector<int> connectionIds() const;
	ClientBrokerConnection* connectionById(int connection_id);
private:
	ClientBrokerConnection* createServerConnection(QWebSocket *socket, QObject *parent);
	void onNewConnection();
	void onConnectionDeleted(int connection_id);
private:
	std::map<int, ClientBrokerConnection*> m_connections;
};

} // namespace rpc
