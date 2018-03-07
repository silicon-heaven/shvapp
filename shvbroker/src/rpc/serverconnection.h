#pragma once

#include <shv/iotqt/rpc/serverconnection.h>

namespace rpc {

class ServerConnection : public shv::iotqt::rpc::ServerConnection
{
	Q_OBJECT

	using Super = shv::iotqt::rpc::ServerConnection;

	SHV_FIELD_IMPL(std::string, m, M, ountPoint)
	public:
		ServerConnection(QTcpSocket* socket, QObject *parent = 0);

	const shv::chainpack::RpcValue& device() const {return m_device;}
private:
	std::string passwordHash(const std::string &user);
	void onRpcDataReceived(shv::chainpack::Rpc::ProtocolType protocol_version, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len) override;
	bool login(const shv::chainpack::RpcValue &auth_params) override;
private:
	shv::chainpack::RpcValue m_device;
	std::string m_user;
	std::string m_pendingAuthNonce;
	bool m_helloReceived = false;
	bool m_loginReceived = false;
};

} // namespace rpc
