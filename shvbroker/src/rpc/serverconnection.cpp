#include "serverconnection.h"

#include "../brokerapp.h"

#include <shv/coreqt/log.h>

#include <QCryptographicHash>
#include <QTcpSocket>
#include <QTimer>

#define logRpcMsg() shvCDebug("RpcMsg")

namespace cp = shv::chainpack;

namespace rpc {

ServerConnection::ServerConnection(QTcpSocket *socket, QObject *parent)
	: Super(socket, parent)
{
	connect(socket, &QTcpSocket::disconnected, this, &ServerConnection::deleteLater);
	connect(this, &ServerConnection::socketConnectedChanged, [this](bool is_connected) {
		if(is_connected) {
			m_helloReceived = m_loginReceived = false;
		}
	});
}

shv::chainpack::RpcValue ServerConnection::deviceId() const
{
	const shv::chainpack::RpcValue::Map &device = connectionOptions().value(cp::Rpc::TYPE_DEVICE).toMap();
	return device.value("id");
}

void ServerConnection::setIdleWatchDogTimeOut(unsigned sec)
{
	if(sec == 0) {
		shvInfo() << "connection ID:" << connectionId() << "switchong idle watch dog timeout OFF";
		if(m_idleWatchDogTimer) {
			delete m_idleWatchDogTimer;
			m_idleWatchDogTimer = nullptr;
		}
	}
	else {
		if(!m_idleWatchDogTimer) {
			m_idleWatchDogTimer = new QTimer(this);
			connect(m_idleWatchDogTimer, &QTimer::timeout, [this]() {
				shvError() << "Connection was idle for more than" << m_idleWatchDogTimer->interval()/1000 << "sec. It will be aborted.";
				this->abort();
			});
		}
		shvInfo() << "connection ID:" << connectionId() << "setting idle watch dog timeout to" << sec << "seconds";
		m_idleWatchDogTimer->start(sec * 1000);
	}
}

std::string ServerConnection::passwordHash(const std::string &user)
{
	if(user == "iot")
		return std::string();
	if(user == "revitest")
		return std::string();

	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(user.c_str(), user.length());
	QByteArray sha1 = hash.result().toHex();
	return std::string(sha1.constData(), sha1.length());
}

void ServerConnection::onRpcDataReceived(shv::chainpack::Rpc::ProtocolType protocol_version, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len)
{
	logRpcMsg() << RCV_LOG_ARROW << md.toStdString() << shv::chainpack::Utils::toHexElided(data, start_pos, 100);
	try {
		if(isInitPhase()) {
			Super::onRpcDataReceived(protocol_version, std::move(md), data, start_pos, data_len);
			return;
		}
		if(m_idleWatchDogTimer)
			m_idleWatchDogTimer->start();
		cp::RpcMessage::setProtocolType(md, protocol_version);
		std::string msg_data(data, start_pos, data_len);
		BrokerApp::instance()->onRpcDataReceived(connectionId(), std::move(md), std::move(msg_data));
	}
	catch (std::exception &e) {
		shvError() << e.what();
	}
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
		m_connectionType = params.value("type").toString();
		m_connectionOptions = params.value("options");
		const shv::chainpack::RpcValue::Map &opts = m_connectionOptions.toMap();
		shv::chainpack::RpcValue::UInt t = opts.value(cp::Rpc::OPT_IDLE_WD_TIMEOUT).toUInt();
		setIdleWatchDogTimeOut(t);
		BrokerApp::instance()->onClientLogin(connectionId());
	}
	return password_ok;
}

} // namespace rpc
