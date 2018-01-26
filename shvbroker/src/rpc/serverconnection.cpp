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

#define logRpc() shvCDebug("rpc")

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
	connect(this, &ServerConnection::connectedChanged, [this](bool is_connected) {
		if(!is_connected)
			this->deleteLater();
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
	rq.setId(id);
	rq.setMethod(method);
	rq.setParams(params);
	sendMessage(rq.value());
	return id;
}

QString ServerConnection::peerAddress() const
{
	if(m_socket)
		return m_socket->peerAddress().toString();
	return QString();
}

void ServerConnection::sendHello()
{
	setAgentName(QStringLiteral("%1:%2").arg(peerAddress()).arg(m_socket->peerPort()));
	shvInfo() << "sending hello to:" << agentName() << "profile:" << m_profile;
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
	m_helloRequestId = callMethodASync("hello", params);
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
	if(!m_knockknockReceived) {
		cp::RpcValue val = decodeData(protocol_version, data, start_pos);
		val.setMetaData(std::move(md));
		cp::RpcMessage msg(val);
		if(msg.isNotify()) {
			cp::RpcNotify ntf(msg);
			shvInfo() << "=> RPC notify received:" << ntf.toStdString();
			if(ntf.method() == shv::chainpack::Rpc::KNOCK_KNOCK) {
				m_knockknockReceived = true;
				const shv::chainpack::RpcValue::Map m = ntf.params().toMap();
				//cp::RpcDriver::ProtocolVersion ver = (cp::RpcDriver::ProtocolVersion)m.value("procolVersion", (unsigned)cp::RpcDriver::ProtocolVersion::ChainPack).toInt();
				const shv::chainpack::RpcValue::String profile = m.value("profile").toString();
				shvInfo() << "Client is knocking, profile:" << profile;// << "device id::" << m.value("deviceId").toStdString();
				//rpcConnection()->setProtocolVersion(ver);
				//emit knockknocReceived(connectionId(), m);
				m_profile = profile;
				sendHello();
				return;
			}
		}
		shvError() << "First message other than" << cp::Rpc::KNOCK_KNOCK << "received!";
		this->deleteLater();
	}
	if(m_helloRequestId > 0 || !m_pendingAuthNonce.empty()) {
		cp::RpcValue val = decodeData(protocol_version, data, start_pos);
		val.setMetaData(std::move(md));
		cp::RpcMessage msg(val);
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
					m_user = login.value("user").toString();
					m_deviceId = login.value("deviceId");

					std::string password_hash = passwordHash(m_user);
					shvInfo() << "login - user:" << m_user << "password:" << password_hash;
					bool password_ok = password_hash.empty();
					if(!password_ok) {
						std::string nonce_sha1 = result.value("nonce").toString();
						std::string nonce = m_pendingAuthNonce + passwordHash(m_user);
						QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
						hash.addData(nonce.c_str(), nonce.length());
						std::string sha1 = std::string(hash.result().toHex().constData());
						shvInfo() << nonce_sha1 << "vs" << sha1;
						password_ok = (nonce_sha1 == sha1);
					}
					if(password_ok && !m_user.empty()) {
						m_helloRequestId = 0;
						m_pendingAuthNonce.clear();
						shvInfo() << "Client logged in user:" << m_user << "from:" << agentName();
						if(BrokerApp::instance()->onClientLogin(connectionId())) {
							shvError() << "Client refused by application.";
							this->deleteLater();
						}
						/*
						if(user == "timepress") {
							processor::Revitest *rv = new processor::Revitest(this);
							connect(m_rpcConnection, &shv::coreqt::chainpack::RpcConnection::messageReceived, rv, &processor::Revitest::onRpcMessageReceived);
							connect(rv, &processor::Revitest::sendRpcMessage, m_rpcConnection, &shv::coreqt::chainpack::RpcConnection::sendMessage);
						}
						*/
					}
					else {
						shvError() << "Invalid autentication for user:" << m_user << "at:" << agentName();
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
	/*
	cp::RpcValue embrio_val;
	embrio_val.setMetaData(std::move(md));
	cp::RpcMessage embrio(embrio_val);
	embrio.setProtocolVersion(protocol_version);
	//shv::chainpack::RpcValue shv_path = embrio.shvPath();
	//embrio.setShvPath(cp::RpcValue());
	*/
	cp::RpcValue::MetaData meta_data(std::move(md));
	cp::RpcMessage::setProtocolVersion(meta_data, protocol_version);
	cp::RpcMessage::setConnectionId(meta_data, connectionId());
	std::string msg_data(data, start_pos, data_len);
	BrokerApp::instance()->onRpcDataReceived(meta_data, msg_data);
}
/*
void ServerConnection::onRpcMessageReceived(const cp::RpcMessage &msg)
{
	logRpc() << msg.toStdString();
	cp::RpcMessage msg2(msg);
	msg2.setConnectionId(connectionId());
	emit rpcMessageReceived(msg2);
}
*/
} // namespace rpc


