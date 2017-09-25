#pragma once

#include "../shvcoreqtglobal.h"

#include <shv/core/chainpack/rpcdriver.h>

#include <QObject>

class QTcpSocket;
class QThread;

namespace shv { namespace core { namespace chainpack { class RpcRequest; class RpcResponse; }}}

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
	void connectToHost(const QString &host_name, quint16 port);

	void sendMessage(const shv::core::chainpack::RpcValue &msg) {Super::sendMessage(msg);}

	Q_SIGNAL void messageReceived(shv::core::chainpack::RpcValue msg);
	void sendRequestSync(const shv::core::chainpack::RpcRequest& request, shv::core::chainpack::RpcResponse *presponse, int time_out_ms);

	void abortConnection();
	bool isConnected() const;
	Q_SIGNAL void connectedChanged(bool is_connected);

protected:
	// RpcDriver interface
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
	bool m_isConnected = false;
};

}}}

Q_DECLARE_METATYPE(shv::core::chainpack::RpcValue)
