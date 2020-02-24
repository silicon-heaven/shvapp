#ifndef SINGLERECORDSETLOGREQUEST_H
#define SINGLERECORDSETLOGREQUEST_H

#include "asyncrequest.h"
#include <shv/core/utils/shvmemoryjournal.h>
#include <QDateTime>

class SingleRecordSetLogRequest : public AsyncRequest
{
	Q_OBJECT
	using Super = AsyncRequest;

public:
	SingleRecordSetLogRequest(const QString &shv_path, const QDateTime &since, const QDateTime &until, QObject *parent);
	void exec() override;

	shv::core::utils::ShvMemoryJournal &receivedLog() { return m_log; }

private:
	void shvCall(const QString &shv_path, const QString &method, const shv::chainpack::RpcValue &params, ResultHandler callback);

	void askDevice(VoidCallback callback);
	void askElesysProvider(VoidCallback callback);
	shv::core::utils::ShvGetLogParams logParams() const;

	QString m_shvPath;
	QDateTime m_since;
	QDateTime m_until;
	shv::core::utils::ShvMemoryJournal m_log;
};

#endif // SINGLERECORDSETLOGREQUEST_H
