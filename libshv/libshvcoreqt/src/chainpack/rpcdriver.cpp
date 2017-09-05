#include "rpcdriver.h"

#include <shv/core/shvexception.h>
#include <shv/core/log.h>
//#include <shv/core/chainpack/metatypes.h>
#include <shv/core/chainpack/rpcmessage.h>
//#include <shv/core/chainpack/chainpackprotocol.h>

#include <QTimer>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QTcpSocket>

//#include <sstream>
//#include <iostream>

//#define smcDebug shvWarning // syn metod call debug
#define smcDebug QNoDebug

#define logRpc() shvCDebug("rpc")
#define logRpcSyncCalls() shvCDebug("RpcSyncCalls")
#define logLongFiles() shvCDebug("LongFiles")

namespace shv {
namespace coreqt {
namespace chainpack {

RpcDriver::RpcDriver(QObject *parent)
	: QObject(parent)
{
	setMessageReceivedCallback([this](const shv::core::chainpack::RpcValue &msg) {
		emit messageReceived(msg);
	});
}

RpcDriver::~RpcDriver()
{
}

void RpcDriver::setSocket(QTcpSocket *socket)
{
	socket->moveToThread(this->thread());
	m_socket = socket;
	connect(socket, &QTcpSocket::readyRead, this, &RpcDriver::onReadyRead);
	connect(socket, &QTcpSocket::bytesWritten, this, &RpcDriver::onBytesWritten, Qt::QueuedConnection);
	connect(socket, &QTcpSocket::connected, [this]() {
		shvDebug() << this << "Connected!!!";
		m_isConnected = true;
		emit connectedChanged(m_isConnected);
	});
	connect(socket, &QTcpSocket::disconnected, [this]() {
		shvDebug() << this << "Disconnected!!!";
		m_isConnected = false;
		emit connectedChanged(m_isConnected);
	});
}

QTcpSocket *RpcDriver::socket()
{
	if(!m_socket)
		SHV_EXCEPTION("Socket is NULL!");
	return m_socket;
}

bool RpcDriver::isConnected() const
{
	return m_socket && m_isConnected;
}

void RpcDriver::connectToHost(const QString &host_name, quint16 port)
{
	socket()->connectToHost(host_name, port);
}

void RpcDriver::onReadyRead()
{
	QByteArray ba = socket()->readAll();
	bytesRead(ba.toStdString());
}

void RpcDriver::onBytesWritten()
{
	logRpc() << "onBytesWritten()";
	writePendingData();
}

bool RpcDriver::isOpen()
{
	return m_socket && m_socket->isOpen();
}

size_t RpcDriver::bytesToWrite()
{
	return socket()->bytesToWrite();
}

int64_t RpcDriver::writeBytes(const char *bytes, size_t length)
{
	return socket()->write(bytes, length);
}

bool RpcDriver::flushNoBlock()
{
	if(m_socket)
		return m_socket->flush();
	return false;
}

namespace {
class ConnectionScope
{
public:
	explicit ConnectionScope(QMetaObject::Connection &c) : m_connection(c) {}
	~ConnectionScope() {if(m_connection) QObject::disconnect(m_connection);}
private:
	QMetaObject::Connection &m_connection;
};
}

void RpcDriver:: sendRequestSync(const core::chainpack::RpcRequest &request, core::chainpack::RpcResponse *presponse, int time_out_ms)
{
	namespace cp = shv::core::chainpack;
	smcDebug() << Q_FUNC_INFO << "timeout ms:" << time_out_ms;
	if(time_out_ms == 0) {
		smcDebug() << "sendMessageSync called with invalid timeout:" << time_out_ms << "," << defaultRpcTimeout() << "msec will be used instead.";
		time_out_ms = defaultRpcTimeout();
	}
	core::chainpack::RpcResponse resp_msg;
	do {
		cp::RpcValue::UInt msg_id = request.id();
		if(msg_id == 0) {
			shvWarning() << "Attempt to send RPC request with ID not set, message will be ignored";
			break;
		}
		smcDebug() << "sending message:" << request.toStdString();
		logRpcSyncCalls() << "--> SEND SYNC message id:" << msg_id << "json:" << request.toStdString();
		sendMessage(request.value());
		QElapsedTimer tm_elapsed;
		tm_elapsed.start();
		QEventLoop eloop;
		QMetaObject::Connection lambda_connection;
		lambda_connection = connect(this, &RpcDriver::messageReceived, [&eloop, &resp_msg, &lambda_connection, msg_id](const shv::core::chainpack::RpcValue &msg_val)
		{
			shv::core::chainpack::RpcMessage msg(msg_val);
			smcDebug() << &eloop << "New RPC message id:" << msg.id();
			if(msg.id() == msg_id) {
				QObject::disconnect(lambda_connection);
				lambda_connection = QMetaObject::Connection();
				resp_msg = msg;
				smcDebug() << "\tID MATCH stopping event loop ..." << &eloop;
				eloop.quit();
			}
			else if(msg.id() == 0) {
				//logRpcSyncCalls() << "<=== RECEIVE NOTIFY while waiting for SYNC response:" << msg.jsonRpcMessage().toString();
			}
			else {
				logRpcSyncCalls() << "RECV other message while waiting for SYNC response id:" << msg.id() << "json:" << msg.toStdString();
			}
		});
		ConnectionScope cscp(lambda_connection);
		if(time_out_ms > 0)
			QTimer::singleShot(time_out_ms, &eloop, &QEventLoop::quit);

		smcDebug() << "\t entering event loop ..." << &eloop;
		eloop.exec();
		smcDebug() << "\t event loop" << &eloop << "exec exit, message received:" << resp_msg.isValid();
		logRpcSyncCalls() << "<-- RECV SYNC message id:" << resp_msg.id() << "json:" << resp_msg.toStdString();
		int elapsed = (int)tm_elapsed.elapsed();
		if(elapsed >= time_out_ms) {
			cp::RpcValue::String err_msg = "Receive message timeout after: " + std::to_string(elapsed) + " msec!";
			shvError() << err_msg;
			auto err = cp::RpcResponse::Error::createInternalError(err_msg);
			resp_msg.setId(msg_id);
			resp_msg.setError(err);
		}
		if(!resp_msg.isValid()) {
			cp::RpcValue::String err_msg = "Invalid mesage returned from sync call!";
			shvError() << err_msg;

			auto err = cp::RpcResponse::Error::createInternalError(err_msg);
			resp_msg.setId(msg_id);
			resp_msg.setError(err);
		}
	} while(false);
	if(presponse)
		*presponse = resp_msg;
}

}}}
