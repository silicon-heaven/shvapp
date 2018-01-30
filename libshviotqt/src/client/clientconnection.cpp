#include "clientconnection.h"
//#include "../theapp.h"

#include <shv/coreqt/chainpack/rpcconnection.h>
#include <shv/coreqt/log.h>

#include <shv/core/shvexception.h>

#include <shv/chainpack/chainpackprotocol.h>
#include <shv/chainpack/rpcmessage.h>

#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QCryptographicHash>

#define logRpc() shvCDebug("rpc")

namespace cp = shv::chainpack;
namespace cpq = shv::coreqt::chainpack;

namespace shv {
namespace iotqt {
namespace client {

Connection::Connection(QObject *parent)
	: Super(cpq::RpcConnection::SyncCalls::Supported, parent)
{
	QTcpSocket *socket = new QTcpSocket();
	setSocket(socket);

	connect(this, &Connection::socketConnectedChanged, this, &Connection::onSocketConnectedChanged);
	connect(this, &Connection::messageReceived, this, &Connection::onRpcMessageReceived);
	//setProtocolVersion(protocolVersion());
	/*
	{
		m_rpcConnection = new shv::coreqt::chainpack::RpcConnection(cpq::RpcConnection::SyncCalls::Supported, this);
		m_rpcConnection->setSocket(socket);
		m_rpcConnection->setProtocolVersion(protocolVersion());
	}
	*/
}

Connection::~Connection()
{
	shvDebug() << Q_FUNC_INFO;
	abort();
}

void Connection::onSocketConnectedChanged(bool is_connected)
{
	if(is_connected) {
		shvInfo() << "Socket connected to RPC server";
		//sendKnockKnock(cp::RpcDriver::ChainPack);
		sendHello();
	}
	else {
		shvInfo() << "Socket disconnected from RPC server";
	}
}

void Connection::sendHello()
{
	setBrokerConnected(false);
	m_helloRequestId = callMethodASync(cp::Rpc::METH_HELLO);
									   //cp::RpcValue::Map{{"profile", profile()}
																//, {"deviceId", deviceId()}
																//, {"protocolVersion", protocolVersion()}
									   //});
	QTimer::singleShot(5000, this, [this]() {
		if(!isBrokerConnected()) {
			shvError() << "Login time out! Dropping client connection.";
			this->deleteLater();
		}
	});
}

void Connection::sendLogin(const chainpack::RpcValue &server_hello)
{
	std::string server_nonce = server_hello.toMap().value("nonce").toString();
	std::string password = server_nonce + passwordHash(user());
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(password.c_str(), password.length());
	QByteArray sha1 = hash.result().toHex();
	m_loginRequestId = callMethodASync(cp::Rpc::METH_LOGIN
									   , cp::RpcValue::Map {
										   {"login", cp::RpcValue::Map {
												{"user", user().toStdString()},
												{"password", std::string(sha1.constData())},
											}
										   },
										   {"device", device()},
									   });
}

std::string Connection::passwordHash(const QString &user)
{
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(user.toUtf8());
	QByteArray sha1 = hash.result().toHex();
	return std::string(sha1.constData(), sha1.length());
}

bool Connection::onRpcMessageReceived(const cp::RpcMessage &msg)
{
	logRpc() << msg.toStdString();
	if(Super::onRpcMessageReceived(msg))
		return true;
	if(!isBrokerConnected()) {
		do {
			if(!msg.isResponse())
				break;
			cp::RpcResponse resp(msg);
			shvInfo() << "Handshake response received:" << resp.toStdString();
			if(resp.isError())
				break;
			unsigned id = resp.id();
			if(id == 0)
				break;
			if(m_helloRequestId == id) {
				sendLogin(resp.result());
				return true;
			}
			else if(m_loginRequestId == id) {
				setBrokerConnected(true);
				return true;
			}
		} while(false);
		shvError() << "Invalid handshake message! Dropping connection." << msg.toStdString();
		this->deleteLater();
		return true;
	}
	return false;
}
#if 0
void Connection::lublicatorTesting()
{
	if(!rpcConnection()->isConnected())
		return;
	try {
		shvInfo() << "==================================================";
		shvInfo() << "   Lublicator Testing";
		shvInfo() << "==================================================";
		{
			shvInfo() << "------------ read shv tree";
			QString shv_path;
			while(true) {
				shvInfo() << "\tcall:" << "get" << "on shv path:" << shv_path;
				cp::RpcResponse resp = rpcConnection()->callShvMethodSync(shv_path, cp::RpcMessage::METHOD_GET);
				shvInfo() << "\tgot response:" << resp.toStdString();
				if(resp.isError())
					throw shv::core::Exception(resp.error().message());
				const cp::RpcValue::List list = resp.result().toList();
				if(list.empty())
					break;
				shv_path += '/' + QString::fromStdString(list[0].toString());
			}
			shvInfo() << "GOT:" << shv_path;
		}
		{
			shvInfo() << "------------ set battery level";
			QString shv_path_lubl = "/shv/eu/pl/lublin/odpojovace/15/";
			for(auto prop : {"status", "batteryLimitLow", "batteryLimitHigh", "batteryLevelSimulation"}) {
				QString shv_path = shv_path_lubl + prop;
				cp::RpcResponse resp = rpcConnection()->callShvMethodSync(shv_path, cp::RpcMessage::METHOD_GET);
				shvInfo() << "\tproperty" << prop << ":" << resp.result().toStdString();
			}
			{
				QString shv_path = shv_path_lubl + "batteryLevelSimulation";
				for (int val = 200; val < 260; val += 5) {
					cp::RpcValue::Decimal dec_val(val, 1);
					shvInfo() << "\tcall:" << "set" << dec_val.toString() << "on shv path:" << shv_path;
					cp::RpcResponse resp = rpcConnection()->callShvMethodSync(shv_path, cp::RpcMessage::METHOD_SET, dec_val);
					shvInfo() << "\tgot response:" << resp.toStdString();
					if(resp.isError())
						throw shv::core::Exception(resp.error().message());
				}
			}
			for(auto prop : {"status", "batteryLimitLow", "batteryLimitHigh", "batteryLevelSimulation"}) {
				QString shv_path = shv_path_lubl + prop;
				cp::RpcResponse resp = rpcConnection()->callShvMethodSync(shv_path, cp::RpcMessage::METHOD_GET);
				shvInfo() << "\tproperty" << prop << ":" << resp.result().toStdString();
			}
			{
				QString shv_path = shv_path_lubl + "batteryLevelSimulation";
				rpcConnection()->callShvMethodSync(shv_path, cp::RpcMessage::METHOD_SET, cp::RpcValue::Decimal(240, 1));
			}
		}
	}
	catch (shv::core::Exception &e) {
		shvError() << "FAILED:" << e.message();
	}
}
#endif
}}}


