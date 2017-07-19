#pragma once

#include "../shvcoreqtglobal.h"

#include <shv/core/chainpack/rpcdriver.h>

#include <QObject>

#include <deque>

class QTcpSocket;
class QThread;


namespace shv {
namespace coreqt {
namespace chainpack {

class SHVCOREQT_DECL_EXPORT RpcDriver : public QObject, public shv::core::chainpack::RpcDriver
{
	Q_OBJECT

	using Super = shv::core::chainpack::RpcDriver;
public:
	explicit RpcDriver(QObject *parent = nullptr);
	~RpcDriver() Q_DECL_OVERRIDE;

	void setSocket(QTcpSocket *socket);
	Q_SIGNAL void messageReceived(shv::core::chainpack::RpcValue msg);
	// RpcDriver interface
protected:
	bool isOpen() Q_DECL_OVERRIDE;
	size_t bytesToWrite() Q_DECL_OVERRIDE;
	int64_t writeBytes(const char *bytes, size_t length) Q_DECL_OVERRIDE;
	bool flushNoBlock() Q_DECL_OVERRIDE;
private:
	QTcpSocket* socket();
	void onReadyRead();
	void onBytesWritten();
private:
	QTcpSocket *m_socket = nullptr;
};

}}}

Q_DECLARE_METATYPE(shv::core::chainpack::RpcValue)
