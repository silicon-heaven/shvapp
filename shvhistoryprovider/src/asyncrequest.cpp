#include "application.h"
#include "asyncrequest.h"

#include <shv/iotqt/rpc/rpc.h>
#include <shv/coreqt/log.h>

namespace cp = shv::chainpack;

const QString &AsyncRequest::errorString() const
{
	return m_error;
}

void AsyncRequest::error(const QString &message)
{
	m_error = message;
	Q_EMIT finished(false);
}

void AsyncRequest::execRequest(AsyncRequest *request, VoidCallback callback)
{
	connect(request, &AsyncRequest::finished, this, [this, request, callback](bool success) {
		request->deleteLater();
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
	}, Qt::QueuedConnection);
	request->exec();
}
