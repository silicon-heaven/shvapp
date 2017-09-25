#include "clientconnection.h"
#include "../theapp.h"

#include <shv/coreqt/chainpack/rpcconnection.h>
#include <shv/coreqt/log.h>

#include <shv/core/chainpack/chainpackprotocol.h>
#include <shv/core/chainpack/rpcmessage.h>

#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QCryptographicHash>

#define logRpc() shvCDebug("rpc")

namespace cp = shv::core::chainpack;

namespace rpc {

ClientConnection::ClientConnection(QObject *parent)
	: Super(parent)
	//, m_socket(new QTcpSocket(this))
{
	QTcpSocket *socket = new QTcpSocket();
	//connect(socket, &QTcpSocket::disconnected, this, &ClientConnection::deleteLater);
	//connect(socket, &QTcpSocket::stateChanged, this, &ClientConnection::onStateChanged);
	//connect(socket, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), this, &ClientConnection::onConnectError);
	{
		m_rpcConnection = new shv::coreqt::chainpack::RpcConnection(this);
		m_rpcConnection->setSocket(socket);
		connect(m_rpcConnection, &shv::coreqt::chainpack::RpcConnection::connectedChanged, this, &ClientConnection::onConnectedChanged);
		connect(m_rpcConnection, &shv::coreqt::chainpack::RpcConnection::messageReceived, this, &ClientConnection::processRpcMessage);
	}
}

ClientConnection::~ClientConnection()
{
	shvDebug() << Q_FUNC_INFO;
}

void ClientConnection::onConnectedChanged(bool is_connected)
{
	if(is_connected) {
		shvInfo() << "Connected to RPC server";
	}
	else {
		shvInfo() << "Disconnected from RPC server";
	}
}
/*
void ClientConnection::onStateChanged(QAbstractSocket::SocketState socket_state)
{
	shvInfo() << "Connection state changed" << QMetaEnum::fromType<QAbstractSocket::SocketState>().valueToKey(socket_state);
}

void ClientConnection::onConnectError(QAbstractSocket::SocketError socket_error)
{
	shvInfo() << "Connection error:" << QMetaEnum::fromType<QAbstractSocket::SocketError>().valueToKey(socket_error);
}

QString ClientConnection::peerAddress() const
{
	return m_socket->peerAddress().toString();
}
*/
shv::coreqt::chainpack::RpcConnection *ClientConnection::rpcConnection() const
{
	return m_rpcConnection;
}

void ClientConnection::connectToHost(const QString& address, int port)
{
	shv::coreqt::chainpack::RpcConnection *conn = rpcConnection();
	shvInfo() << "Connecting to" << address << port;
	conn->connectToHost(address, port);
}

bool ClientConnection::isConnected() const
{
	return rpcConnection()->isConnected();
}

void ClientConnection::abort()
{
	rpcConnection()->abort();
}

std::string ClientConnection::passwordHash(const QString &user)
{
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(user.toUtf8());
	QByteArray sha1 = hash.result().toHex();
	return std::string(sha1.constData(), sha1.length());
}

void ClientConnection::processRpcMessage(const cp::RpcMessage &msg)
{
	logRpc() << msg.toStdString();
	if(m_isWaitingForHello && (msg.isNotify() || msg.isResponse())) {
		shvError() << "HELLO request invalid! Dropping connection.";
		this->deleteLater();
		return;
	}
	if(msg.isRequest()) {
		cp::RpcRequest rq(msg);
		if(rq.isNotify()) {
			return;
		}
		try {
			if(m_isWaitingForHello) {
				if(rq.method() == "hello") {
					cp::RpcValue::Map params = rq.params().toMap();
					//QString server_profile = params.value(QStringLiteral("profile")).toString();
					//Application::instance()->setServerProfile(server_profile);
					//m_clientId = params.value(QStringLiteral("clientId")).toInt();
					std::string challenge = params.value("challenge").toString();
					challenge += passwordHash(user());
					QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
					hash.addData(challenge.c_str(), challenge.length());
					QByteArray sha1 = hash.result().toHex();
					cp::RpcValue::Map result{
						{"challenge", std::string(sha1.constData())},
						{"login", cp::RpcValue::Map{ {"user", user().toStdString()}, }},
					};
					rpcConnection()->sendResponse(rq.id(), result);
					m_isWaitingForHello = false;
					shvInfo() << "Sending HELLO to RPC server";
				}
				else {
					shvError() << "HELLO request invalid! Dropping connection." << rq.method();
					this->deleteLater();
				}
				return;
			}
		}
		catch(const shv::core::Exception &e) {
			shvError() << "process RPC request exception:" << e.message();
			cp::RpcResponse::Error err;
			err.setMessage(e.message());
			err.setCode(cp::RpcResponse::Error::MethodInvocationException);
			//err.setData(e.where() + "\n----- stack trace -----\n" + e.stackTrace());
			rpcConnection()->sendError(rq.id(), err);
		}
	}
	else {
		shvError() << "unhandled response";
	}
}

} // namespace rpc


