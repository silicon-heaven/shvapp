#ifndef ASYNCREQUEST_H
#define ASYNCREQUEST_H

#include <QObject>

#include <shv/chainpack/rpcvalue.h>

using ResultHandler = std::function<void(const shv::chainpack::RpcValue &)>;
using BoolCallback = std::function<void(bool)>;
using VoidCallback = std::function<void()>;

class AsyncRequest : public QObject
{
	Q_OBJECT

public:
	AsyncRequest(QObject *parent) : QObject(parent) {}

	Q_SIGNAL void finished(bool);
	const QString &errorString() const;

	virtual void exec() = 0;

protected:
	void execRequest(AsyncRequest *request, VoidCallback callback);
	void error(const QString &message);

	QString m_error;
};

#endif // ASYNCREQUEST_H
