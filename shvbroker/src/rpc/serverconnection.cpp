#include "serverconnection.h"
#include "../brokerapp.h"

#include <shv/coreqt/chainpack/rpcconnection.h>
#include <shv/coreqt/log.h>

//#include <shv/chainpack/chainpackprotocol.h>
#include <shv/chainpack/rpcmessage.h>

#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QCryptographicHash>

#define logRpcMsg() shvCDebug("RpcMsg")

namespace cp = shv::chainpack;
namespace cpq = shv::coreqt::chainpack;

namespace rpc {

static int s_connectionId = 0;

ServerConnection::ServerConnection(QTcpSocket *socket, QObject *parent)
	: Super(parent)
	//, m_socket(socket)
	, m_connectionId(++s_connectionId)
{
	setSocket(socket);
	socket->setParent(nullptr);
	connect(this, &ServerConnection::socketConnectedChanged, [this](bool is_connected) {
		if(!is_connected) {
			this->deleteLater();
		}
		else {
			setAgentName(QStringLiteral("%1:%2").arg(peerAddress()).arg(m_socket->peerPort()));
			m_helloReceived = m_loginReceived = false;
		}
	});
}

ServerConnection::~ServerConnection()
{
	shvInfo() << "Agent disconnected:" << agentName();
}

namespace {
int nextRpcId()
{
	static int n = 0;
	return ++n;
}
}
int ServerConnection::callMethodASync(const std::string & method, const cp::RpcValue &params)
{
	int id = nextRpcId();
	cp::RpcRequest rq;
	rq.setRequestId(id);
	rq.setMethod(method);
	rq.setParams(params);
	sendMessage(rq.value());
	return id;
}

void ServerConnection::sendResponse(int request_id, const cp::RpcValue &result)
{
	cp::RpcResponse resp;
	resp.setRequestId(request_id);
	resp.setResult(result);
	sendMessage(resp.value());
}

void ServerConnection::sendError(int request_id, const cp::RpcResponse::Error &error)
{
	cp::RpcResponse resp;
	resp.setRequestId(request_id);
	resp.setError(error);
	sendMessage(resp.value());
}

QString ServerConnection::peerAddress() const
{
	if(m_socket)
		return m_socket->peerAddress().toString();
	return QString();
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

void ServerConnection::onRpcDataReceived(shv::chainpack::Rpc::ProtocolVersion protocol_version, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len)
{
	if(!m_helloReceived || !m_loginReceived) {
		cp::RpcValue val = decodeData(protocol_version, data, start_pos);
		val.setMetaData(std::move(md));
		cp::RpcMessage msg(val);
		logRpcMsg() << msg.toCpon();
		if(!msg.isRequest()) {
			shvError() << "Initial message is not RPC request! Dropping client connection." << agentName() << msg.toCpon();
			this->deleteLater();
			return;
		}
		cp::RpcRequest rq(msg);
		try {
			//shvInfo() << "RPC request received:" << rq.toStdString();
			if(!m_helloReceived && !m_loginReceived && rq.method() == "echo" && BrokerApp::instance()->cliOptions()->isEchoEnabled()) {
				shvInfo() << "Client echo request received";// << profile;// << "device id::" << m.value("deviceId").toStdString();
				m_pendingAuthNonce = std::to_string(std::rand());
				sendResponse(rq.requestId(), rq.params());
				return;
			}
			if(!m_helloReceived && !m_loginReceived && rq.method() == shv::chainpack::Rpc::METH_HELLO) {
				shvInfo() << "Client hello received";// << profile;// << "device id::" << m.value("deviceId").toStdString();
				//const shv::chainpack::RpcValue::String profile = m.value("profile").toString();
				//m_profile = profile;
				m_helloReceived = true;
				shvInfo() << "sending hello response:" << agentName();// << "profile:" << m_profile;
				m_pendingAuthNonce = std::to_string(std::rand());
				cp::RpcValue::Map params {
					//{"protocol", cp::RpcValue::Map{{"version", protocol_version}}},
					{"nonce", m_pendingAuthNonce}
				};
				sendResponse(rq.requestId(), params);
				QTimer::singleShot(3000, this, [this]() {
					if(!m_loginReceived) {
						shvError() << "client login time out! Dropping client connection." << agentName();
						this->deleteLater();
					}
				});
				return;
			}
			if(m_helloReceived && !m_loginReceived && rq.method() == shv::chainpack::Rpc::METH_LOGIN) {
				shvInfo() << "Client login received";// << profile;// << "device id::" << m.value("deviceId").toStdString();
				m_loginReceived = true;
				cp::RpcValue::Map params = rq.params().toMap();
				const cp::RpcValue::Map login = params.value("login").toMap();
				m_user = login.value("user").toString();
				m_device = params.value("device");

				std::string password_hash = passwordHash(m_user);
				shvInfo() << "login - user:" << m_user << "password:" << password_hash;
				bool password_ok = password_hash.empty();
				if(!password_ok) {
					std::string nonce_sha1 = login.value("password").toString();
					std::string nonce = m_pendingAuthNonce + passwordHash(m_user);
					QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
					hash.addData(nonce.c_str(), nonce.length());
					std::string sha1 = std::string(hash.result().toHex().constData());
					shvInfo() << nonce_sha1 << "vs" << sha1;
					password_ok = (nonce_sha1 == sha1);
				}
				if(password_ok && !m_user.empty()) {
					shvInfo() << "Client logged in user:" << m_user << "from:" << agentName();
					BrokerApp::instance()->onClientLogin(connectionId());
					sendResponse(rq.requestId(), true);
				}
				else {
					SHV_EXCEPTION("Invalid authentication for user: " + m_user + " at: " + agentName().toStdString());
				}
				return;
			}
		}
		catch(shv::core::Exception &e) {
			sendError(rq.requestId(), cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodInvocationException, e.message()));
		}
		shvError() << "Initial handshake error! Dropping client connection." << agentName() << msg.toCpon();
		QTimer::singleShot(100, this, &ServerConnection::deleteLater); // need some time to send error to client
		//this->deleteLater();
		return;
	}

	logRpcMsg() << RCV_LOG_ARROW << md.toStdString() << shv::chainpack::Utils::toHexElided(data, start_pos, 100);
	cp::RpcMessage::setProtocolVersion(md, protocol_version);
	if(cp::RpcMessage::isRequest(md))
		cp::RpcMessage::setCallerId(md, connectionId());
	std::string msg_data(data, start_pos, data_len);
	BrokerApp::instance()->onRpcDataReceived(std::move(md), std::move(msg_data));
}

} // namespace rpc


