#ifndef SINGLERECORDSETLOGREQUEST_H
#define SINGLERECORDSETLOGREQUEST_H

#include "abstractrequest.h"
#include <shv/core/utils/shvmemoryjournal.h>
#include <QDateTime>

class SingleRecordSetLogRequest : public AbstractRequest
{
	Q_OBJECT
	using Super = AbstractRequest;

public:
	SingleRecordSetLogRequest(const QString &shv_path, const QDateTime &since, const QDateTime &until, QObject *parent);
	void exec() override;

	shv::core::utils::ShvMemoryJournal &receivedLog() { return m_log; }

private:
	void askDevice(VoidCallback callback);
	void askElesys(VoidCallback callback);
	shv::core::utils::ShvGetLogParams logParams() const;

	QString m_shvPath;
	QDateTime m_since;
	QDateTime m_until;
	shv::core::utils::ShvMemoryJournal m_log;
};

#endif // SINGLERECORDSETLOGREQUEST_H
