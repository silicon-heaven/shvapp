#pragma once

#include "commonrpcclienthandle.h"

#include <shv/iotqt/rpc/serverconnection.h>

#include <set>

class QTimer;

namespace shv { namespace core { class StringView; }}

namespace rpc {

class ServerConnection : public shv::iotqt::rpc::ServerConnection, public CommonRpcClientHandle
{
	Q_OBJECT

	using Super = shv::iotqt::rpc::ServerConnection;

	//SHV_FIELD_IMPL(std::string, m, M, ountPoint)
public:
	ServerConnection(QTcpSocket* socket, QObject *parent = nullptr);
	~ServerConnection() override;

	int connectionId() const override {return Super::connectionId();}
	bool isConnectedAndLoggedIn() const override {return Super::isConnectedAndLoggedIn();}
	bool isSlaveBrokerConnection() const override {return Super::isSlaveBrokerConnection();}
	bool isMasterBrokerConnection() const override {return false;}

	std::string loggedUserName() override {return Super::userName();}

	shv::chainpack::RpcValue tunnelOptions() const;
	shv::chainpack::RpcValue deviceOptions() const;
	shv::chainpack::RpcValue deviceId() const;

	void setIdleWatchDogTimeOut(int sec);

	void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) override;
	void sendRawData(const shv::chainpack::RpcValue::MetaData &meta_data, std::string &&data) override;

	bool propagateSubscriptionToSlaveBroker(const Subscription &subs);
private:
	void onRpcDataReceived(shv::chainpack::Rpc::ProtocolType protocol_type, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len) override;
	std::tuple<std::string, PasswordFormat> password(const std::string &user) override;
	shv::chainpack::RpcValue login(const shv::chainpack::RpcValue &auth_params) override;
	bool checkTunnelSecret(const std::string &s);
private:
	QTimer *m_idleWatchDogTimer = nullptr;
};

} // namespace rpc
