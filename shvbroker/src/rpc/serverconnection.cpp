#include "serverconnection.h"
#include "../theapp.h"

#include <shv/coreqt/chainpack/rpcconnection.h>
#include <shv/coreqt/log.h>

//#include <shv/chainpack/chainpackprotocol.h>
#include <shv/chainpack/rpcmessage.h>

#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QCryptographicHash>

#include <ctime>

#define logRpc() shvCDebug("rpc")

namespace cp = shv::chainpack;
namespace cpq = shv::coreqt::chainpack;

namespace rpc {

static int s_connectionId = 0;

ServerConnection::ServerConnection(QTcpSocket *socket, QObject *parent)
	: Super(parent)
	, m_socket(socket)
	, m_connectionId(++s_connectionId)
{
	std::srand(std::time(nullptr));
	socket->setParent(nullptr);
	connect(m_socket, &QTcpSocket::disconnected, this, &ServerConnection::deleteLater);
	{
		m_rpcConnection = new shv::coreqt::chainpack::RpcConnection(cpq::RpcConnection::SyncCalls::NotSupported, this);
		m_rpcConnection->setSocket(m_socket);
		connect(m_rpcConnection, &cpq::RpcConnection::messageReceived, this, &ServerConnection::onRpcMessageReceived);
	}
}

ServerConnection::~ServerConnection()
{
	shvInfo() << "Agent disconnected:" << agentName();
}

QString ServerConnection::peerAddress() const
{
	return m_socket->peerAddress().toString();
}

shv::coreqt::chainpack::RpcConnection *ServerConnection::rpcConnection()
{
	return m_rpcConnection;
}

void ServerConnection::sendHello()
{
	setAgentName(QStringLiteral("%1:%2").arg(peerAddress()).arg(m_socket->peerPort()));
	shvInfo() << "sending hello to:" << agentName();
	m_pendingAuthNonce = std::to_string(std::rand());
	cp::RpcValue::Map params {
		//{"protocol", cp::RpcValue::Map{{"version", protocol_version}}},
		{"nonce", m_pendingAuthNonce}
	};
	QTimer::singleShot(3000, this, [this]() {
		if(m_helloRequestId > 0 || !m_pendingAuthNonce.empty()) {
			shvError() << "HELLO request client time out! Dropping client connection." << agentName();
			this->deleteLater();
		}
	});
	m_helloRequestId = rpcConnection()->callMethodASync("hello", params);
}

std::string ServerConnection::passwordHash(const std::string &user)
{
	if(user == "iot")
		return std::string();
	if(user == "timepress")
		return std::string();

	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(user.c_str(), user.length());
	QByteArray sha1 = hash.result().toHex();
	return std::string(sha1.constData(), sha1.length());
}

void ServerConnection::onRpcMessageReceived(const cp::RpcMessage &msg)
{
	logRpc() << msg.toStdString();
	if(!m_knockknockReceived) {
		if(msg.isNotify()) {
			cp::RpcNotify ntf(msg);
			shvInfo() << "=> RPC notify received:" << ntf.toStdString();
			if(ntf.method() == shv::chainpack::Rpc::KNOCK_KNOCK) {
				m_knockknockReceived = true;
				const shv::chainpack::RpcValue::Map m = ntf.params().toMap();
				//cp::RpcDriver::ProtocolVersion ver = (cp::RpcDriver::ProtocolVersion)m.value("procolVersion", (unsigned)cp::RpcDriver::ProtocolVersion::ChainPack).toInt();
				const shv::chainpack::RpcValue::String profile = m.value("profile").toString();
				shvInfo() << "Client is knocking, profile:" << profile << "device id::" << m.value("deviceId").toStdString();
				//rpcConnection()->setProtocolVersion(ver);
				sendHello();
				return;
			}
		}
		shvError() << "First message other than" << cp::Rpc::KNOCK_KNOCK << "received!";
		this->deleteLater();
	}
	if(m_helloRequestId > 0 || !m_pendingAuthNonce.empty()) {
		if(msg.isResponse()) {
			cp::RpcResponse resp(msg);
			if(resp.isError()) {
				shvError() << "HELLO request error response!" << resp.error().toString() << "Dropping client connection." << agentName();
				this->deleteLater();
			}
			else {
				if(resp.id() == m_helloRequestId) {
					cp::RpcValue::Map result = resp.result().toMap();
					const cp::RpcValue::Map login = result.value("login").toMap();
					const cp::RpcValue::String user = login.value("user").toString();

					std::string password_hash = passwordHash(user);
					shvInfo() << "login - user:" << user << "password:" << password_hash;
					bool password_ok = password_hash.empty();
					if(!password_ok) {
						std::string nonce_sha1 = result.value("nonce").toString();
						std::string nonce = m_pendingAuthNonce + passwordHash(user);
						QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
						hash.addData(nonce.c_str(), nonce.length());
						std::string sha1 = std::string(hash.result().toHex().constData());
						shvInfo() << nonce_sha1 << "vs" << sha1;
						password_ok = (nonce_sha1 == sha1);
					}
					if(password_ok && !user.empty()) {
						m_helloRequestId = 0;
						m_pendingAuthNonce.clear();
						shvInfo() << "Agent logged in user:" << user << "from:" << agentName();
						/*
						if(user == "timepress") {
							processor::Revitest *rv = new processor::Revitest(this);
							connect(m_rpcConnection, &shv::coreqt::chainpack::RpcConnection::messageReceived, rv, &processor::Revitest::onRpcMessageReceived);
							connect(rv, &processor::Revitest::sendRpcMessage, m_rpcConnection, &shv::coreqt::chainpack::RpcConnection::sendMessage);
						}
						*/
					}
					else {
						shvError() << "Invalid autentication for user:" << user << "at:" << agentName();
						this->deleteLater();
					}
				}
				else {
					shvError() << "HELLO response invalid request id! Dropping client connection." << agentName();
					this->deleteLater();
				}
			}
		}
		else {
			shvError() << "HELLO request invalid response! Dropping client connection." << agentName();
			this->deleteLater();
		}
		return;
	}
	cp::RpcMessage msg2(msg);
	msg2.setConnectionId(connectionId());
	emit rpcMessageReceived(msg2);
}

} // namespace rpc


