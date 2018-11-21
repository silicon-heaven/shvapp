#include "serverconnection.h"

#include "../brokerapp.h"

#include <shv/chainpack/cponwriter.h>
#include <shv/coreqt/log.h>
#include <shv/core/stringview.h>

#include <QCryptographicHash>
#include <QTcpSocket>
#include <QTimer>

#define logRpcMsg() shvCDebug("RpcMsg")

namespace cp = shv::chainpack;

namespace rpc {

ServerConnection::ServerConnection(QTcpSocket *socket, QObject *parent)
	: Super(socket, parent)
{
	connect(this, &ServerConnection::socketConnectedChanged, [this](bool is_connected) {
		if(!is_connected) {
			shvInfo() << "Socket disconnected, deleting connection:" << connectionId();
			deleteLater();
		}
	});
}

ServerConnection::~ServerConnection()
{
	//rpc::ServerConnectionshvWarning() << "destroying" << this;
}

const shv::chainpack::RpcValue::Map& ServerConnection::deviceOptions() const
{
	return connectionOptions().value(cp::Rpc::KEY_DEVICE).toMap();
}

shv::chainpack::RpcValue ServerConnection::deviceId() const
{
	return deviceOptions().value(cp::Rpc::KEY_DEVICE_ID);
}

void ServerConnection::setIdleWatchDogTimeOut(int sec)
{
	if(sec == 0) {
		shvInfo() << "connection ID:" << connectionId() << "switching idle watch dog timeout OFF";
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

std::tuple<std::string, ServerConnection::PasswordFormat> ServerConnection::password(const std::string &user)
{
	/*
	const std::map<std::string, std::string> passwds {
		{"iot", "lub42DUB"},
		{"elviz", "brch3900PRD"},
		{"revitest", "lautrhovno271828"},
	};
	*/
	std::tuple<std::string, ServerConnection::PasswordFormat> invalid;
	if(user.empty())
		return invalid;
	BrokerApp *app = BrokerApp::instance();
	const shv::chainpack::RpcValue::Map &user_def = app->usersConfig().toMap().value(user).toMap();
	if(user_def.empty()) {
		shvWarning() << "Invalid user:" << user;
		return invalid;
	}
	std::string pass = user_def.value("password").toString();
	std::string password_format_str = user_def.value("passwordFormat").toString();
	if(password_format_str.empty()) {
		// obsolete users.cpon files used "loginType" key for passwordFormat
		password_format_str = user_def.value("loginType").toString();
	}
	PasswordFormat password_format = passwordFormatFromString(password_format_str);
	if(password_format == PasswordFormat::Invalid) {
		shvWarning() << "Invalid password format for user:" << user;
		return invalid;
	}
	return std::tuple<std::string, ServerConnection::PasswordFormat>(pass, password_format);
}

void ServerConnection::sendMessage(const shv::chainpack::RpcMessage &rpc_msg)
{
	logRpcMsg() << SND_LOG_ARROW
				<< "client id:" << connectionId()
				<< "protocol_type:" << (int)protocolType() << shv::chainpack::Rpc::protocolTypeToString(protocolType())
				<< rpc_msg.toPrettyString();
	Super::sendMessage(rpc_msg);
}

void ServerConnection::sendRawData(const shv::chainpack::RpcValue::MetaData &meta_data, std::string &&data)
{
	logRpcMsg() << SND_LOG_ARROW
				<< "client id:" << connectionId()
				<< "protocol_type:" << (int)protocolType() << shv::chainpack::Rpc::protocolTypeToString(protocolType())
				<< BrokerApp::instance()->dataToCpon(shv::chainpack::RpcMessage::protocolType(meta_data), meta_data, data, 0);
	Super::sendRawData(meta_data, std::move(data));
}

void ServerConnection::onRpcDataReceived(shv::chainpack::Rpc::ProtocolType protocol_type, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len)
{
	logRpcMsg() << RCV_LOG_ARROW
				<< "client id:" << connectionId()
				<< "protocol_type:" << (int)protocol_type << shv::chainpack::Rpc::protocolTypeToString(protocol_type)
				<< BrokerApp::instance()->dataToCpon(protocol_type, md, data, start_pos);
	try {
		if(isInitPhase()) {
			Super::onRpcDataReceived(protocol_type, std::move(md), data, start_pos, data_len);
			return;
		}
		if(m_idleWatchDogTimer)
			m_idleWatchDogTimer->start();
		std::string msg_data(data, start_pos, data_len);
		BrokerApp::instance()->onRpcDataReceived(connectionId(), protocol_type, std::move(md), std::move(msg_data));
	}
	catch (std::exception &e) {
		shvError() << e.what();
	}
}

shv::chainpack::RpcValue ServerConnection::login(const shv::chainpack::RpcValue &auth_params)
{
	cp::RpcValue ret = Super::login(auth_params);
	if(!ret.isValid())
		return cp::RpcValue();
	cp::RpcValue::Map login_resp = ret.toMap();

	const shv::chainpack::RpcValue::Map &opts = connectionOptions();
	shv::chainpack::RpcValue::UInt t = opts.value(cp::Rpc::OPT_IDLE_WD_TIMEOUT).toUInt();
	setIdleWatchDogTimeOut(t);
	BrokerApp::instance()->onClientLogin(connectionId());

	login_resp[cp::Rpc::KEY_CLIENT_ID] = connectionId();
	return login_resp;
}

/*
shv::chainpack::Rpc::AccessGrant ServerConnection::accessGrantForShvPath(const std::string &shv_path)
{
	shv::chainpack::Rpc::AccessGrant *pag = m_accessGrantCache.object(shv_path);
	if(!pag) {
		BrokerApp *app = BrokerApp::instance();
		shv::chainpack::Rpc::AccessGrant ag = app->accessGrantForShvPath(m_user, shv_path);
		m_accessGrantCache.insert(shv_path, new cp::Rpc::AccessGrant(ag));
		return ag;
	}
	return *pag;
}
*/
} // namespace rpc
