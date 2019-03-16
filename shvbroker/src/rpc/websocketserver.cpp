#include "websocketserver.h"

#include "clientbrokerconnection.h"
#include "websocket.h"
#include "websocketserverconnection.h"

#include <shv/coreqt/log.h>

#include <QWebSocket>

namespace rpc {

WebSocketServer::WebSocketServer(QObject *parent)
	: Super("shvbroker", QWebSocketServer::NonSecureMode, parent)
{
	/*
	 * certificate must be provided for secure mode
	QSslConfiguration sslConfiguration;
	QFile certFile(QStringLiteral(":/localhost.cert"));
	QFile keyFile(QStringLiteral(":/localhost.key"));
	//QFile certFile(QStringLiteral(":/key.pem"));
	//QFile keyFile(QStringLiteral(":/key.key"));
	certFile.open(QIODevice::ReadOnly);
	keyFile.open(QIODevice::ReadOnly);
	QSslCertificate certificate(&certFile, QSsl::Pem);
	QSslKey sslKey(&keyFile, QSsl::Rsa, QSsl::Pem);
	certFile.close();
	keyFile.close();
	sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
	sslConfiguration.setLocalCertificate(certificate);
	sslConfiguration.setPrivateKey(sslKey);
	sslConfiguration.setProtocol(QSsl::TlsV1SslV3);
	m_pWebSocketServer->setSslConfiguration(sslConfiguration);
	*/
	connect(this, &WebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
}

WebSocketServer::~WebSocketServer()
{
}

bool WebSocketServer::start(int port)
{
	quint16 p = port == 0? 3777: static_cast<quint16>(port);
	shvInfo() << "Starting" << (secureMode() == QWebSocketServer::SecureMode? "secure (wss:)": "not-secure (ws:)") << "WebSocket server on port:" << p;
	if (!listen(QHostAddress::AnyIPv4, p)) {
		shvError() << tr("Unable to start the server: %1.").arg(errorString());
		close();
		return false;
	}
	shvInfo().nospace()
			<< "WebSocket RPC server is listenning on "
			<< serverAddress().toString() << ":" << serverPort()
			<< " in " << (secureMode() == QWebSocketServer::SecureMode? " secure (wss:)": " not-secure (ws:)") << " mode";
	return true;
}

std::vector<int> WebSocketServer::connectionIds() const
{
	std::vector<int> ret;
	for(const auto &pair : m_connections)
		ret.push_back(pair.first);
	return ret;
}

ClientBrokerConnection *WebSocketServer::connectionById(int connection_id)
{
	auto it = m_connections.find(connection_id);
	if(it == m_connections.end())
		return nullptr;
	return it->second;
}

ClientBrokerConnection *WebSocketServer::createServerConnection(QWebSocket *socket, QObject *parent)
{
	return new ClientBrokerConnection(new WebSocket(socket), parent);
}

void WebSocketServer::onNewConnection()
{
	shvInfo() << "New WebSocket connection";
	QWebSocket *sock = nextPendingConnection();
	if(sock) {
		shvInfo().nospace() << "web socket client connected: " << sock->peerAddress().toString() << ':' << sock->peerPort();// << "socket:" << sock << sock->socketDescriptor() << "state:" << sock->state();
		ClientBrokerConnection *c = createServerConnection(sock, this);
		c->setConnectionName(sock->peerAddress().toString().toStdString() + ':' + std::to_string(sock->peerPort()));
		m_connections[c->connectionId()] = c;
		int cid = c->connectionId();
		connect(c, &ClientBrokerConnection::destroyed, [this, cid]() {
			onConnectionDeleted(cid);
		});
	}
}

void WebSocketServer::onConnectionDeleted(int connection_id)
{
	m_connections.erase(connection_id);
}

} // namespace rpc
