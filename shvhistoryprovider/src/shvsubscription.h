#ifndef SHVSUBSCRIPTION_H
#define SHVSUBSCRIPTION_H

#include <QObject>

#include <shv/chainpack/rpcvalue.h>
#include <shv/iotqt/rpc/clientconnection.h>


class ShvSubscription : public QObject
{
	Q_OBJECT
	using Super = QObject;

public:
	ShvSubscription(shv::iotqt::rpc::ClientConnection *conn, const QString &path, const QString &method, QObject *parent);
	Q_SIGNAL void notificationReceived(const QString &path, const QString &method, const shv::chainpack::RpcValue &data);

private:
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	QString m_path;
	QString m_method;
};

#endif // SHVSUBSCRIPTION_H
