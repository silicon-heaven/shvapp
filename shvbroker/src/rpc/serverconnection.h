#pragma once

#include <QObject>
//#include <QPointer>

class QTcpSocket;
//class Agent;

namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace coreqt { namespace chainpack { class RpcConnection; }}}

namespace rpc {

class ServerConnection : public QObject
{
	Q_OBJECT

	using Super = QObject;
public:
	explicit ServerConnection(QTcpSocket* socket, QObject *parent = 0);
	~ServerConnection() Q_DECL_OVERRIDE;

	QString agentName() {return objectName();}
	void setAgentName(const QString &n) {setObjectName(n);}
private:
	QString peerAddress() const;
	shv::coreqt::chainpack::RpcConnection* rpcConnection();
	void processRpcMessage(const shv::chainpack::RpcMessage &msg);
	void sendHello(unsigned protocol_version);
	std::string passwordHash(const std::string &user);
private:
	QTcpSocket* m_socket = nullptr;
	shv::coreqt::chainpack::RpcConnection* m_rpcConnection = nullptr;
	std::string m_pendingAuthNonce;
	unsigned m_helloRequestId = 0;
};

} // namespace rpc

