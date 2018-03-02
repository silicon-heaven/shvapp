#include "serverconnection.h"

#include "../brokerapp.h"

#include <shv/coreqt/log.h>

#include <QCryptographicHash>
#include <QTimer>

#define logRpcMsg() shvCDebug("RpcMsg")

namespace cp = shv::chainpack;

namespace rpc {

ServerConnection::ServerConnection(QTcpSocket *socket, QObject *parent)
	: Super(socket, parent)
{
    connect(this, &ServerConnection::socketConnectedChanged, [this](bool is_connected) {
		if(is_connected) {
			m_helloReceived = m_loginReceived = false;
		}
	});
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
	logRpcMsg() << RCV_LOG_ARROW << md.toStdString() << shv::chainpack::Utils::toHexElided(data, start_pos, 100);
	if(isInitPhase()) {
		Super::onRpcDataReceived(protocol_version, std::move(md), data, start_pos, data_len);
		return;
	}
	cp::RpcMessage::setProtocolVersion(md, protocol_version);
	std::string msg_data(data, start_pos, data_len);
	BrokerApp::instance()->onRpcDataReceived(connectionId(), std::move(md), std::move(msg_data));
}

bool ServerConnection::login(const shv::chainpack::RpcValue &auth_params)
{
	const cp::RpcValue::Map params = auth_params.toMap();
	const cp::RpcValue::Map login = params.value("login").toMap();

	m_user = login.value("user").toString();
	if(m_user.empty())
		return false;

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
	if(password_ok) {
		m_device = params.value("device");
		BrokerApp::instance()->onClientLogin(connectionId());
	}
	return password_ok;
}

} // namespace rpc
