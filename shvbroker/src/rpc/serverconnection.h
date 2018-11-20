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

	std::string userName() override {return Super::userName();}

	const shv::chainpack::RpcValue::Map& deviceOptions() const;
	shv::chainpack::RpcValue deviceId() const;

	void setIdleWatchDogTimeOut(int sec);

	void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) override;
	void sendRawData(const shv::chainpack::RpcValue::MetaData &meta_data, std::string &&data) override;
private:
	void onRpcDataReceived(shv::chainpack::Rpc::ProtocolType protocol_type, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len) override;
	bool checkPassword(const shv::chainpack::RpcValue::Map &login) override;
	std::string passwordHash(LoginType type, const std::string &user) override;
	shv::chainpack::RpcValue login(const shv::chainpack::RpcValue &auth_params) override;
private:
	QTimer *m_idleWatchDogTimer = nullptr;
};

} // namespace rpc
