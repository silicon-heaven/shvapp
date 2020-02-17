#ifndef ABSTRACTREQUEST_H
#define ABSTRACTREQUEST_H

#include <QObject>
#include <QVector>

#include <shv/chainpack/rpcvalue.h>
#include <shv/core/utils/shvmemoryjournal.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>

using ResultHandler = std::function<void(const shv::chainpack::RpcValue &)>;
using ResponseHandler = shv::iotqt::rpc::RpcResponseCallBack::CallBackFunction;
using BoolCallback = std::function<void(bool)>;
using VoidCallback = std::function<void()>;

class AbstractRequest : public QObject
{
	Q_OBJECT

public:
	AbstractRequest(QObject *parent) : QObject(parent) {}
	~AbstractRequest();

	Q_SIGNAL void finished(bool);
	const QString &errorString() const;

	virtual void exec() = 0;

protected:
	void shvCall(const QString &shv_path, const QString &method, const shv::chainpack::RpcValue &params, ResultHandler callback);
	void shvCall(const QString &shv_path, const QString &method, const shv::chainpack::RpcValue &params, ResponseHandler callback);
	//void execRequest(AbstractRequest *request, BoolCallback callback);
	//void execRequest(AbstractRequest *request, VoidCallback callback);

	void cancelRunningShvCalls();
	void cancelRunningRequests();
	virtual void error(const QString &message);

	//QVector<shv::iotqt::rpc::RpcResponseCallBack*> m_runningShvCalls;
	//QVector<AbstractRequest*> m_runningRequest;
	QString m_error;
};

#endif // SHVCALLER_H
