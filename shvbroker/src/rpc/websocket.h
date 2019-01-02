#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <shv/iotqt/rpc/socket.h>

class QWebSocket;

namespace rpc {

class WebSocket : public shv::iotqt::rpc::Socket
{
	Q_OBJECT

	using Super = shv::iotqt::rpc::Socket;
public:
	WebSocket(QWebSocket *socket, QObject *parent = nullptr);

	void connectToHost(const QString &host_name, quint16 port) override;
	void close() override;
	void abort() override;
	QAbstractSocket::SocketState state() const override;
	QString errorString() const override;
	QHostAddress peerAddress() const override;
	quint16 peerPort() const override;
	QByteArray readAll() override;
	qint64 write(const char *data, qint64 data_size) override;
	void writeMessageBegin() override;
	void writeMessageEnd() override;
private:
	void onTextMessageReceived(const QString &message);
	void onBinaryMessageReceived(const QByteArray &message);
private:
	QWebSocket *m_socket = nullptr;
	QByteArray m_readBuffer;
	QByteArray m_writeBuffer;
};

} // namespace rpc

#endif // WEBSOCKET_H
