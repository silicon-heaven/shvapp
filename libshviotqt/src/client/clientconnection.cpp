#include "clientconnection.h"
//#include "../theapp.h"

#include <shv/coreqt/chainpack/rpcconnection.h>
#include <shv/coreqt/log.h>

#include <shv/core/shvexception.h>

#include <shv/chainpack/chainpackprotocol.h>
#include <shv/chainpack/rpcmessage.h>

#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QCryptographicHash>

#define logRpc() shvCDebug("rpc")

namespace cp = shv::chainpack;
namespace cpq = shv::coreqt::chainpack;

namespace shv {
namespace iotqt {
namespace client {

Connection::Connection(QObject *parent)
	: Super(cpq::RpcConnection::SyncCalls::Supported, parent)
{
	//setDevice(cp::RpcValue(nullptr));

	QTcpSocket *socket = new QTcpSocket();
	setSocket(socket);

	connect(this, &Connection::socketConnectedChanged, this, &Connection::onSocketConnectedChanged);
	connect(this, &Connection::messageReceived, this, &Connection::onRpcMessageReceived);
	//setProtocolVersion(protocolVersion());
	/*
	{
		m_rpcConnection = new shv::coreqt::chainpack::RpcConnection(cpq::RpcConnection::SyncCalls::Supported, this);
		m_rpcConnection->setSocket(socket);
		m_rpcConnection->setProtocolVersion(protocolVersion());
	}
	*/
}

Connection::~Connection()
{
	shvDebug() << Q_FUNC_INFO;
	abort();
}

void Connection::onSocketConnectedChanged(bool is_connected)
{
	if(is_connected) {
		shvInfo() << "Socket connected to RPC server";
		//sendKnockKnock(cp::RpcDriver::ChainPack);
		//RpcResponse resp = callMethodSync("echo", "ahoj babi");
		//shvInfo() << "+++" << resp.toStdString();
		sendHello();
	}
	else {
		shvInfo() << "Socket disconnected from RPC server";
	}
}

void Connection::sendHello()
{
	setBrokerConnected(false);
	m_helloRequestId = callMethodASync(cp::Rpc::METH_HELLO);
									   //cp::RpcValue::Map{{"profile", profile()}
																//, {"deviceId", deviceId()}
																//, {"protocolVersion", protocolVersion()}
									   //});
	QTimer::singleShot(5000, this, [this]() {
		if(!isBrokerConnected()) {
			shvError() << "Login time out! Dropping client connection.";
			this->deleteLater();
		}
	});
}

void Connection::sendLogin(const chainpack::RpcValue &server_hello)
{
	std::string server_nonce = server_hello.toMap().value("nonce").toString();
	std::string password = server_nonce + passwordHash(user());
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(password.c_str(), password.length());
	QByteArray sha1 = hash.result().toHex();
	m_loginRequestId = callMethodASync(cp::Rpc::METH_LOGIN
									   , cp::RpcValue::Map {
										   {"login", cp::RpcValue::Map {
												{"user", user()},
												{"password", std::string(sha1.constData())},
											}
										   },
										   {"device", device()},
									   });
}

std::string Connection::passwordHash(const std::string &user)
{
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(user.data(), user.size());
	QByteArray sha1 = hash.result().toHex();
	return std::string(sha1.constData(), sha1.length());
}

bool Connection::onRpcMessageReceived(const cp::RpcMessage &msg)
{
	logRpc() << msg.toStdString();
	if(Super::onRpcMessageReceived(msg))
		return true;
	if(!isBrokerConnected()) {
		do {
			if(!msg.isResponse())
				break;
			cp::RpcResponse resp(msg);
			shvInfo() << "Handshake response received:" << resp.toStdString();
			if(resp.isError())
				break;
			unsigned id = resp.requestId();
			if(id == 0)
				break;
			if(m_helloRequestId == id) {
				sendLogin(resp.result());
				return true;
			}
			else if(m_loginRequestId == id) {
				setBrokerConnected(true);
				return true;
			}
		} while(false);
		shvError() << "Invalid handshake message! Dropping connection." << msg.toStdString();
		this->deleteLater();
		return true;
	}
	return false;
}

}}}


