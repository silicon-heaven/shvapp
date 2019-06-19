#include "clientbrokerconnection.h"
#include "masterbrokerconnection.h"

#include "../brokerapp.h"

#include <shv/chainpack/cponwriter.h>
#include <shv/coreqt/log.h>
#include <shv/core/stringview.h>
#include <shv/iotqt/utils/shvpath.h>
#include <shv/iotqt/rpc/socket.h>

#include <QCryptographicHash>
#include <QTcpSocket>
#include <QTimer>

#define logSubscriptionsD() nCDebug("Subscr").color(NecroLog::Color::Yellow)
#define logSubsResolveD() nCDebug("SubsRes").color(NecroLog::Color::LightGreen)

namespace cp = shv::chainpack;

namespace rpc {

ClientBrokerConnection::ClientBrokerConnection(shv::iotqt::rpc::Socket *socket, QObject *parent)
	: Super(socket, parent)
{
	connect(this, &ClientBrokerConnection::socketConnectedChanged, this, &ClientBrokerConnection::onSocketConnectedChanged);
}

ClientBrokerConnection::~ClientBrokerConnection()
{
	//rpc::ServerConnectionshvWarning() << "destroying" << this;
	//shvWarning() << __FUNCTION__;
}

void ClientBrokerConnection::onSocketConnectedChanged(bool is_connected)
{
	if(!is_connected) {
		shvInfo() << "Socket disconnected, deleting connection:" << connectionId();
		deleteLater();
	}
}

shv::chainpack::RpcValue ClientBrokerConnection::tunnelOptions() const
{
	return connectionOptions().value(cp::Rpc::KEY_TUNNEL);
}

shv::chainpack::RpcValue ClientBrokerConnection::deviceOptions() const
{
	return connectionOptions().value(cp::Rpc::KEY_DEVICE);
}

shv::chainpack::RpcValue ClientBrokerConnection::deviceId() const
{
	return deviceOptions().toMap().value(cp::Rpc::KEY_DEVICE_ID);
}

void ClientBrokerConnection::addMountPoint(const std::string &mp)
{
	m_mountPoints.push_back(mp);
}

void ClientBrokerConnection::setIdleWatchDogTimeOut(int sec)
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
				std::string mount_points = shv::core::String::join(mountPoints(), ", ");
				shvError() << "Connection id:" << connectionId() << "device id:" << deviceId().toCpon() << "mount points:" << mount_points
						   << "was idle for more than" << m_idleWatchDogTimer->interval()/1000 << "sec. It will be aborted.";
				this->abort();
			});
		}
		shvInfo() << "connection ID:" << connectionId() << "setting idle watch dog timeout to" << sec << "seconds";
		m_idleWatchDogTimer->start(sec * 1000);
	}
}

std::tuple<std::string, ClientBrokerConnection::PasswordFormat> ClientBrokerConnection::password(const std::string &user)
{
	/*
	const std::map<std::string, std::string> passwds {
		{"iot", "lub42DUB"},
		{"elviz", "brch3900PRD"},
		{"revitest", "lautrhovno271828"},
	};
	*/
	std::tuple<std::string, ClientBrokerConnection::PasswordFormat> invalid;
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
	return std::tuple<std::string, ClientBrokerConnection::PasswordFormat>(pass, password_format);
}

void ClientBrokerConnection::sendMessage(const shv::chainpack::RpcMessage &rpc_msg)
{
	logRpcMsg() << SND_LOG_ARROW
				<< "client id:" << connectionId()
				<< "protocol_type:" << (int)protocolType() << shv::chainpack::Rpc::protocolTypeToString(protocolType())
				<< rpc_msg.toPrettyString();
	Super::sendMessage(rpc_msg);
}

void ClientBrokerConnection::sendRawData(const shv::chainpack::RpcValue::MetaData &meta_data, std::string &&data)
{
	logRpcMsg() << SND_LOG_ARROW
				<< "client id:" << connectionId()
				<< "protocol_type:" << (int)protocolType() << shv::chainpack::Rpc::protocolTypeToString(protocolType())
				<< BrokerApp::instance()->dataToCpon(shv::chainpack::RpcMessage::protocolType(meta_data), meta_data, data);
	Super::sendRawData(meta_data, std::move(data));
}

std::string ClientBrokerConnection::resolveLocalPath(const std::string rel_path)
{
	if(!shv::iotqt::utils::ShvPath::isRelativePath(rel_path))
		return rel_path;

	const std::vector<std::string> &mps = mountPoints();
	if(mps.empty())
		SHV_EXCEPTION("Cannot resolve relative path on unmounted device: " + rel_path);
	if(mps.size() > 1)
		SHV_EXCEPTION("Cannot resolve relative path on device mounted to more than single node: " + rel_path);
	std::string mount_point = mps[0];
	MasterBrokerConnection *mbconn = BrokerApp::instance()->mainMasterBrokerConnection();
	if(mbconn) {
		/// if the client is mounted on exported path,
		/// then relative path must be resolved with respect to it
		mount_point = mbconn->localPathToMasterExported(mount_point);
	}
	std::string local_path = shv::iotqt::utils::ShvPath::joinAndClean(mount_point, rel_path);
	if(!shv::iotqt::utils::ShvPath::isRelativePath(local_path) && mbconn) {
		/// not relative path after join
		/// no need to send it to the master broker, still local path,
		/// prepend exported path
		local_path = shv::iotqt::utils::ShvPath::join(mbconn->exportedShvPath(), local_path);
	}
	return local_path;
}

unsigned ClientBrokerConnection::addSubscription(const std::string &rel_path, const std::string &method)
{
	Subscription subs = shv::iotqt::utils::ShvPath::isRelativePath(rel_path)?
				Subscription{resolveLocalPath(rel_path), rel_path, method}:
				Subscription{rel_path, std::string(), method};
	return CommonRpcClientHandle::addSubscription(subs);
}

bool ClientBrokerConnection::removeSubscription(const std::string &rel_path, const std::string &method)
{
	Subscription subs = shv::iotqt::utils::ShvPath::isRelativePath(rel_path)?
				Subscription{resolveLocalPath(rel_path), rel_path, method}:
				Subscription{rel_path, std::string(), method};
	return CommonRpcClientHandle::removeSubscription(subs);
}

void ClientBrokerConnection::onRpcDataReceived(shv::chainpack::Rpc::ProtocolType protocol_type, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len)
{
	logRpcMsg() << RCV_LOG_ARROW
				<< "client id:" << connectionId()
				<< "protocol_type:" << (int)protocol_type << shv::chainpack::Rpc::protocolTypeToString(protocol_type)
				<< BrokerApp::instance()->dataToCpon(protocol_type, md, data, start_pos, data_len);
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

shv::chainpack::RpcValue ClientBrokerConnection::login(const shv::chainpack::RpcValue &auth_params)
{
	cp::RpcValue ret;
	if(tunnelOptions().isMap()) {
		std::string secret = tunnelOptions().toMap().value(cp::Rpc::KEY_SECRET).toString();
		if(checkTunnelSecret(secret))
			ret = cp::RpcValue::Map();
	}
	else {
		ret = Super::login(auth_params);
	}
	if(!ret.isValid())
		return cp::RpcValue();
	cp::RpcValue::Map login_resp = ret.toMap();

	const shv::chainpack::RpcValue::Map &opts = connectionOptions();
	shv::chainpack::RpcValue::UInt t = opts.value(cp::Rpc::OPT_IDLE_WD_TIMEOUT).toUInt();
	setIdleWatchDogTimeOut(t);
	BrokerApp::instance()->onClientLogin(connectionId());

	login_resp[cp::Rpc::KEY_CLIENT_ID] = connectionId();
	return shv::chainpack::RpcValue{login_resp};
}

bool ClientBrokerConnection::checkTunnelSecret(const std::string &s)
{
	return BrokerApp::instance()->checkTunnelSecret(s);
}

void ClientBrokerConnection::propagateSubscriptionToSlaveBroker(const CommonRpcClientHandle::Subscription &subs)
{
	if(!isSlaveBrokerConnection())
		return;
	for(const std::string &mount_point : mountPoints()) {
		if(shv::iotqt::utils::ShvPath(subs.absolutePath).startsWithPath(mount_point)) {
			std::string slave_path = subs.absolutePath.substr(mount_point.size());
			if(!slave_path.empty() && slave_path[0] == '/')
				slave_path = slave_path.substr(1);
			callMethodSubscribe(slave_path, subs.method, cp::Rpc::GRANT_MASTER_BROKER);
			return;
		}
		if(shv::iotqt::utils::ShvPath(mount_point).startsWithPath(subs.absolutePath)) {
			callMethodSubscribe(std::string(), subs.method, cp::Rpc::GRANT_MASTER_BROKER);
			return;
		}
	}
}

} // namespace rpc
