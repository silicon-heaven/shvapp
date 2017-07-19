#include "rpcdriver.h"

#include <shv/core/shvexception.h>
#include <shv/core/log.h>
//#include <shv/core/chainpack/metatypes.h>
//#include <shv/core/chainpack/rpcmessage.h>
//#include <shv/core/chainpack/chainpackprotocol.h>

#include <QTcpSocket>

//#include <sstream>
//#include <iostream>

#define logRpc() shvCDebug("rpc")
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
	m_socket = socket;
	connect(socket, &QTcpSocket::readyRead, this, &RpcDriver::onReadyRead);
	connect(socket, &QTcpSocket::bytesWritten, this, &RpcDriver::onBytesWritten, Qt::QueuedConnection);
}

QTcpSocket *RpcDriver::socket()
{
	if(!m_socket)
		SHV_EXCEPTION("Socket is NULL!");
	return m_socket;
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

}}}
