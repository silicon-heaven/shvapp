#include "rpcconnection.h"

#include <shv/core/shvexception.h>
#include <shv/core/log.h>

#include <QThread>

#define logRpc() shvCDebug("rpc")
#define logRpcSyncCalls() shvCDebug("RpcSyncCalls")

namespace shv {
namespace coreqt {
namespace chainpack {

RpcConnection::RpcConnection(QObject *parent)
	: QObject(parent)
{
	static int id = 0;
	m_connectionId = ++id;
	m_rpcDriverThread = new QThread();
	m_rpcDriver = new RpcDriver();
	//m_rpcDriver->setSocket(new QTcpSocket(m_rpcDriver));
	m_rpcDriver->moveToThread(m_rpcDriverThread);

	connect(this, &RpcConnection::sendMessageRequest, m_rpcDriver, &RpcDriver::sendMessage, Qt::QueuedConnection);
	connect(this, &RpcConnection::sendMessageSyncRequest, m_rpcDriver, &RpcDriver::sendRequestSync, Qt::BlockingQueuedConnection);
	connect(this, &RpcConnection::connectToHostRequest, m_rpcDriver, &RpcDriver::connectToHost, Qt::QueuedConnection);

	connect(m_rpcDriver, &RpcDriver::connectedChanged, this, &RpcConnection::connectedChanged, Qt::QueuedConnection);
	connect(m_rpcDriver, &RpcDriver::messageReceived, this, &RpcConnection::onMessageReceived, Qt::QueuedConnection);
	//connect(m_socketDriver, &RpcDriver::rpcError, this, &RpcConnection::rpcError, Qt::QueuedConnection);

	connect(m_rpcDriverThread, &QThread::finished, [this]() {
		delete this->m_rpcDriver;
		this->m_rpcDriver = nullptr;
	});

	m_rpcDriverThread->start();
}

RpcConnection::~RpcConnection()
{
}

void RpcConnection::setSocket(QTcpSocket *socket)
{
	m_rpcDriver->setSocket(socket);
}

void RpcConnection::connectToHost(const QString &host_name, quint16 port)
{
	emit connectToHostRequest(host_name, port);
}

bool RpcConnection::isConnected() const
{
	return m_rpcDriver->isConnected();
}

void RpcConnection::onMessageReceived(const RpcConnection::RpcValue &rpc_val)
{
	RpcMessage msg(rpc_val);
	RpcValue::UInt id = msg.id();
	if(id > 0 && id <= m_maxSyncMessageId) {
		// ignore messages alredy processed by sync calls
		logRpcSyncCalls() << "<xxx ignoring already served sync response:" << id;
		return;
	}
	emit messageReceived(msg);
}

void RpcConnection::sendMessage(const RpcConnection::RpcMessage &rpc_msg)
{
	emit sendMessageRequest(rpc_msg.value());
}

RpcConnection::RpcResponse RpcConnection::sendMessageSync(const RpcConnection::RpcRequest &rpc_request_message, int time_out_ms)
{
	RpcResponse res_msg;
	m_maxSyncMessageId = qMax(m_maxSyncMessageId, rpc_request_message.id());
	logRpcSyncCalls() << "==> send SYNC MSG id:" << rpc_request_message.id() << "data:" << rpc_request_message.toStdString();
	emit sendMessageSyncRequest(rpc_request_message, &res_msg, time_out_ms);
	logRpcSyncCalls() << "<== RESP SYNC MSG id:" << rpc_request_message.id() << "data:" << rpc_request_message.toStdString();
	return res_msg;
}

void RpcConnection::sendNotify(const QString &method, const RpcConnection::RpcValue &params)
{
	RpcRequest rq;
	rq.setMethod(method.toStdString());
	rq.setParams(params);
	sendMessage(rq);
}

void RpcConnection::sendResponse(int request_id, const RpcConnection::RpcValue &result)
{
	RpcResponse resp;
	resp.setId(request_id);
	resp.setResult(result);
	sendMessage(resp);
}

void RpcConnection::sendError(int request_id, const core::chainpack::RpcResponse::Error &error)
{
	RpcResponse resp;
	resp.setId(request_id);
	resp.setError(error);
	sendMessage(resp);
}

namespace {
int nextRpcId()
{
	static int n = 0;
	return ++n;
}
}
int RpcConnection::callMethodASync(const QString &method, const RpcConnection::RpcValue &params)
{
	int id = nextRpcId();
	RpcRequest rq;
	rq.setId(id);
	rq.setMethod(method.toStdString());
	rq.setParams(params);
	sendMessage(rq);
	return id;
}

RpcConnection::RpcResponse RpcConnection::callMethodSync(const QString &method, const RpcConnection::RpcValue &params, int rpc_timeout)
{
	RpcRequest rq;
	rq.setId(nextRpcId());
	rq.setMethod(method.toStdString());
	rq.setParams(params);
	//logRpc() << "--> sync method call:" << id << method;
	RpcMessage ret = sendMessageSync(rq, rpc_timeout);
	if(!ret.isResponse())
		SHV_EXCEPTION("Invalid response!");
	//logRpc() << "<-- sync method call ret:" << ret.id();
	return RpcResponse(ret);
}

} // namespace chainpack
} // namespace coreqt
} // namespace shv
