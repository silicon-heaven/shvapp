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
//namespace cpq = shv::coreqt::chainpack;

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
		m_rpcConnection->setProtocolVersion(cp::RpcDriver::Cpon);
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
		//sendKnockKnock(cp::RpcDriver::ChainPack);
		sendKnockKnock(cp::RpcDriver::Cpon);
		m_isWaitingForHello = true;
	}
	else {
		shvInfo() << "Disconnected from RPC server";
	}
}

void ClientConnection::sendKnockKnock(int protocol_version, const std::string &profile)
{
	rpcConnection()->sendNotify("knockknock", cp::RpcValue::Map{{"protocolVersion", protocol_version}, {"profile", profile}});
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
					std::string nonce = params.value("nonce").toString();
					nonce += passwordHash(user());
					QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
					hash.addData(nonce.c_str(), nonce.length());
					QByteArray sha1 = hash.result().toHex();
					cp::RpcValue::Map result{
						{"nonce", std::string(sha1.constData())},
						{"login", cp::RpcValue::Map{ {"user", user().toStdString()}, }},
					};
					rpcConnection()->sendResponse(rq.id(), result);
					m_isWaitingForHello = false;
					shvInfo() << "Sending HELLO to RPC server";
					QTimer::singleShot(1000, this, &ClientConnection::lublicatorTesting);
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

void ClientConnection::lublicatorTesting()
{
	if(!rpcConnection()->isConnected())
		return;
	try {
		shvInfo() << "==================================================";
		shvInfo() << "   Lublicator Testing";
		shvInfo() << "==================================================";
		{
			shvInfo() << "------------ read shv tree";
			QString shv_path;
			while(true) {
				shvInfo() << "\tcall:" << "get" << "on shv path:" << shv_path;
				cp::RpcResponse resp = rpcConnection()->callShvMethodSync(shv_path, "get");
				shvInfo() << "\tgot response:" << resp.toStdString();
				if(resp.isError())
					throw shv::core::Exception(resp.error().message());
				const cp::RpcValue::List list = resp.result().toList();
				if(list.empty())
					break;
				shv_path += '/' + QString::fromStdString(list[0].toString());
			}
			shvInfo() << "GOT:" << shv_path;
		}
	}
	catch (shv::core::Exception &e) {
		shvError() << "FAILED:" << e.message();
	}
}

} // namespace rpc


