#pragma once

#include <shv/coreqt/chainpack/rpcdriver.h>

#include <shv/chainpack/rpcmessage.h>
#include <shv/core/utils.h>

#include <QObject>

#include <string>

class QTcpSocket;
//class Agent;

//namespace shv { namespace chainpack { class RpcMessage; class RpcValue; }}
//namespace shv { namespace coreqt { namespace chainpack { class RpcConnection; }}}

namespace rpc {

class ServerConnection : public shv::coreqt::chainpack::RpcDriver
{
	Q_OBJECT

	using Super = shv::coreqt::chainpack::RpcDriver;

	SHV_FIELD_IMPL(std::string, m, M, ountPoint)
public:
	explicit ServerConnection(QTcpSocket* socket, QObject *parent = 0);
	~ServerConnection() Q_DECL_OVERRIDE;

	QString agentName() {return objectName();}
	void setAgentName(const QString &n) {setObjectName(n);}

	int connectionId() const {return m_connectionId;}
	const shv::chainpack::RpcValue& device() const {return m_device;}

	int callMethodASync(const std::string &method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue());
	void sendResponse(int request_id, const shv::chainpack::RpcValue &result);
	void sendError(int request_id, const shv::chainpack::RpcResponse::Error &error);

	//Q_SIGNAL void knockknocReceived(int connection_id, const shv::chainpack::RpcValue::Map &params);
	//Q_SIGNAL void rpcDataReceived(const shv::chainpack::RpcValue::MetaData &meta, const std::string &data);
	//Q_SIGNAL void rpcMessageReceived(const shv::chainpack::RpcMessage &msg);
protected:
	void onRpcDataReceived(shv::chainpack::Rpc::ProtocolVersion protocol_version, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len) override;
private:
	QString peerAddress() const;
	//void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	std::string passwordHash(const std::string &user);
private:
	//QTcpSocket* m_socket = nullptr;
	//ServerRpcDriver* m_rpcDriver = nullptr;
	//unsigned m_helloRequestId = 0;
	std::string m_pendingAuthNonce;
	bool m_helloReceived = false;
	bool m_loginReceived = false;
	//std::string m_profile;
	std::string m_user;
	shv::chainpack::RpcValue m_device;
	int m_connectionId;
};

} // namespace rpc

