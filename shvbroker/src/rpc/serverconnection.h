#pragma once

#include <QObject>
//#include <QPointer>

class QTcpSocket;
//class Agent;

namespace shv { namespace core { namespace chainpack { class RpcMessage; }}}
namespace shv { namespace coreqt { namespace chainpack { class RpcConnection; }}}

namespace rpc {

class ServerConnection : public QObject
{
	Q_OBJECT

	using Super = QObject;
public:
	explicit ServerConnection(QTcpSocket* socket, QObject *parent = 0);
	QString peerAddress() const;
private:
	shv::coreqt::chainpack::RpcConnection* rpcConnection();
	void processRpcMessage(const shv::core::chainpack::RpcMessage &msg);
	void sendHello();
	std::string passwordHash(const std::string &user);
private:
	QTcpSocket* m_socket = nullptr;
	shv::coreqt::chainpack::RpcConnection* m_rpcConnection = nullptr;
	std::string m_pendingAuthChallenge;
	unsigned m_helloRequestId = 0;
};

} // namespace rpc

