#pragma once

#include <shv/coreqt/chainpack/rpcdriver.h>

#include <shv/chainpack/rpcmessage.h>

#include <QObject>

class QTcpSocket;
//class Agent;

//namespace shv { namespace chainpack { class RpcMessage; class RpcValue; }}
//namespace shv { namespace coreqt { namespace chainpack { class RpcConnection; }}}

namespace rpc {

class ServerConnection : public shv::coreqt::chainpack::RpcDriver
{
	Q_OBJECT

	using Super = shv::coreqt::chainpack::RpcDriver;
public:
	explicit ServerConnection(QTcpSocket* socket, QObject *parent = 0);
	~ServerConnection() Q_DECL_OVERRIDE;

	QString agentName() {return objectName();}
	void setAgentName(const QString &n) {setObjectName(n);}

	int connectionId() const {return m_connectionId;}

	int callMethodASync(const std::string &method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue());

	Q_SIGNAL void rpcDataReceived(const shv::chainpack::RpcValue::MetaData &meta, const std::string &data);
	//Q_SIGNAL void rpcMessageReceived(const shv::chainpack::RpcMessage &msg);
protected:
	void onRpcDataReceived(shv::chainpack::Rpc::ProtocolVersion protocol_version, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len) override;
private:
	QString peerAddress() const;
	//void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void sendHello();
	std::string passwordHash(const std::string &user);
private:
	//QTcpSocket* m_socket = nullptr;
	//ServerRpcDriver* m_rpcDriver = nullptr;
	std::string m_pendingAuthNonce;
	unsigned m_helloRequestId = 0;
	bool m_knockknockReceived = false;
	int m_connectionId;
};

} // namespace rpc

