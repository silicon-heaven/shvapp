#pragma once

#include <shv/core/utils.h>

#include <QAbstractSocket>
#include <QObject>

class QTcpSocket;

namespace shv { namespace core { namespace chainpack { class RpcMessage; }}}
namespace shv { namespace coreqt { namespace chainpack { class RpcConnection; }}}

namespace rpc {

class ClientConnection : public QObject
{
	Q_OBJECT
	using Super = QObject;

	SHV_FIELD_IMPL(QString, u, U, ser)
	//SHV_FIELD_IMPL(QString, p, P, asswordHash)
public:
	explicit ClientConnection(QObject *parent = 0);
	~ClientConnection() Q_DECL_OVERRIDE;

	void connectToHost(const QString& address, int port);
	//QString peerAddress() const;
	bool isConnected() const;
	void abort();
private:
	shv::coreqt::chainpack::RpcConnection* rpcConnection() const;
	std::string passwordHash(const QString &user);
	void processRpcMessage(const shv::core::chainpack::RpcMessage &msg);
	//void onStateChanged(QAbstractSocket::SocketState socket_state);
	//void onConnectError(QAbstractSocket::SocketError socket_error);
	void onConnectedChanged(bool is_connected);
private:
	//QTcpSocket* m_socket = nullptr;
	shv::coreqt::chainpack::RpcConnection* m_rpcConnection = nullptr;
	//std::string m_pendingAuthChallenge;
	bool m_isWaitingForHello = true;
};

} // namespace rpc

