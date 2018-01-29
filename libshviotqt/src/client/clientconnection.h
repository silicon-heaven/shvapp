#pragma once

#include "../shviotqtglobal.h"

#include <shv/core/utils.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/rpcdriver.h>

#include <QAbstractSocket>
#include <QObject>

namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace coreqt { namespace chainpack { class RpcConnection; }}}

namespace shv {
namespace iotqt {
namespace client {
/*
class HelloResponse : public shv::chainpack::RpcValue
{
public:
	void setUser(const std::string &u);
	void setPassword(const std::string &p);
	void setDeviceId(const shv::chainpack::RpcValue &id);
	void setDeviceMountPoint(const shv::chainpack::RpcValue::String &path);
};
*/
class SHVIOTQT_DECL_EXPORT Connection : public QObject
{
	Q_OBJECT
	using Super = QObject;

	SHV_FIELD_IMPL(QString, u, U, ser)
	SHV_FIELD_IMPL(shv::chainpack::Rpc::ProtocolVersion, p, P, rotocolVersion)
	SHV_FIELD_IMPL(std::string, p, P, rofile)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, d, D, evice)
public:
	explicit Connection(QObject *parent = 0);
	~Connection() Q_DECL_OVERRIDE;

	void connectToHost(const QString& address, int port);
	//QString peerAddress() const;
	bool isSocketConnected() const;
	void abort();
private:
	shv::coreqt::chainpack::RpcConnection* rpcConnection();
	std::string passwordHash(const QString &user);
	void processRpcMessage(const shv::chainpack::RpcMessage &msg);
	void onSocketConnectedChanged(bool is_connected);
	void sendKnockKnock();
private:
	//void lublicatorTesting();
private:
	shv::coreqt::chainpack::RpcConnection* m_rpcConnection = nullptr;
	bool m_isWaitingForHello = true;
};

}}}

