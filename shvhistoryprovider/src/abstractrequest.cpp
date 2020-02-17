#include "application.h"
#include "abstractrequest.h"

#include <shv/iotqt/rpc/rpc.h>
#include <shv/coreqt/log.h>

namespace cp = shv::chainpack;

AbstractRequest::~AbstractRequest()
{
	cancelRunningShvCalls();
	cancelRunningRequests();
}

const QString &AbstractRequest::errorString() const
{
	return m_error;
}

void AbstractRequest::cancelRunningShvCalls()
{
	for (auto *cb : m_runningShvCalls) {
		cb->deleteLater();
	}
	m_runningShvCalls.clear();
}

void AbstractRequest::cancelRunningRequests()
{
	for (AbstractRequest *request : m_runningRequest) {
		request->deleteLater();
	}
	m_runningRequest.clear();
}

void AbstractRequest::error(const QString &message)
{
	cancelRunningShvCalls();
	cancelRunningRequests();
	m_error = message;
	Q_EMIT finished(false);
}

void AbstractRequest::shvCall(const QString &shv_path, const QString &method, const cp::RpcValue &params, ResultHandler callback)
{
	shvCall(shv_path, method, params, [this, callback](const cp::RpcResponse &response) {
		if (response.isError()) {
			error(QString::fromStdString(response.error().message()));
		}
		else {
			try {
				callback(response.result());
			}
			catch (const shv::core::Exception &e) {
				error(e.what());
			}
		}
	});
}

void AbstractRequest::shvCall(const QString &shv_path, const QString &method, const shv::chainpack::RpcValue &params, ResponseHandler callback)
{
	auto *conn = Application::instance()->deviceConnection();
	int rq_id = conn->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rq_id, this);
	m_runningShvCalls << cb;
	cb->start([this, cb, callback](const cp::RpcResponse &resp) {
		m_runningShvCalls.removeOne(cb);
		try {
			callback(resp);
		}
		catch (const shv::core::Exception &e) {
			error(e.what());
		}
	});
	conn->callShvMethod(rq_id, shv_path.toStdString(), method.toStdString(), params);
}

void AbstractRequest::execRequest(AbstractRequest *request, BoolCallback callback)
{
	m_runningRequest << request;
	connect(request, &AbstractRequest::finished, this, [this, request, callback](bool success) {
		m_runningRequest.removeOne(request);
		request->deleteLater();
		try {
			callback(success);
		}
		catch (const shv::core::Exception &e) {
			error(e.what());
		}
	}, Qt::QueuedConnection);
	request->exec();
}

void AbstractRequest::execRequest(AbstractRequest *request, VoidCallback callback)
{
	execRequest(request, [this, request, callback](bool success) {
		if (!success) {
			error(request->errorString());
			return;
		}
		try {
			callback();
		}
		catch (const shv::core::Exception &e) {
			error(e.what());
		}
	});
}
