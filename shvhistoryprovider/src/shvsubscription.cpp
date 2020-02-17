#include "shvsubscription.h"

ShvSubscription::ShvSubscription(shv::iotqt::rpc::ClientConnection *conn, const QString &path, const QString &method, QObject *parent)
	: Super(parent)
	, m_path(path)
	, m_method(method)
{
	connect(conn, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvSubscription::onRpcMessageReceived);
}

void ShvSubscription::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	if (msg.isSignal()) {
		shv::chainpack::RpcSignal ntf(msg);
		QString path = QString::fromStdString(ntf.shvPath().toString());
		QString method = QString::fromStdString(ntf.method().toString());
		if (path.startsWith(m_path) && (m_method.isEmpty() || m_method == method)) {
			Q_EMIT notificationReceived(path, method, ntf.params());
		}
	}
}
