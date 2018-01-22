#pragma once

#include <shv/core/utils.h>

#include <QAbstractSocket>
#include <QObject>

class QTcpSocket;

namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace coreqt { namespace chainpack { class RpcConnection; }}}

namespace rpc {

class ClientConnection : public QObject
{
	Q_OBJECT
	using Super = QObject;

	SHV_FIELD_IMPL(QString, u, U, ser)
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
	void processRpcMessage(const shv::chainpack::RpcMessage &msg);
	void onConnectedChanged(bool is_connected);
	void sendKnockKnock(int protocol_version, const std::string &profile = std::string());
private:
	void lublicatorTesting();
private:
	shv::coreqt::chainpack::RpcConnection* m_rpcConnection = nullptr;
	bool m_isWaitingForHello = true;
};

}

