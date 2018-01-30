#pragma once

#include "../shviotqtglobal.h"

#include <shv/core/utils.h>
#include <shv/coreqt/utils.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/rpcdriver.h>

#include <shv/coreqt/chainpack/rpcconnection.h>

#include <QAbstractSocket>
#include <QObject>

namespace shv { namespace chainpack { class RpcMessage; }}

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
class SHVIOTQT_DECL_EXPORT Connection : public shv::coreqt::chainpack::RpcConnection
{
	Q_OBJECT
	using Super = shv::coreqt::chainpack::RpcConnection;

	SHV_FIELD_IMPL(QString, u, U, ser)
	//SHV_FIELD_IMPL(std::string, p, P, rofile)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, d, D, evice)

	SHV_PROPERTY_BOOL_IMPL(b, B, rokerConnected)
public:
	explicit Connection(QObject *parent = 0);
	~Connection() Q_DECL_OVERRIDE;
protected:
	bool onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) override;
private:
	std::string passwordHash(const QString &user);
	void onSocketConnectedChanged(bool is_connected);
	void sendHello();
	void sendLogin(const shv::chainpack::RpcValue &server_hello);
private:
	//void lublicatorTesting();
private:
	unsigned m_helloRequestId = 0;
	unsigned m_loginRequestId = 0;
};

}}}

