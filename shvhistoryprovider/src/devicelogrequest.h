#ifndef DEVICELOGREQUEST_H
#define DEVICELOGREQUEST_H

#include "asyncrequest.h"
#include "logdir.h"

#include <shv/core/utils/shvmemoryjournal.h>
#include <QDateTime>

class DeviceLogRequest : public AsyncRequest
{
	Q_OBJECT
	using Super = AsyncRequest;

public:
	explicit DeviceLogRequest(const QString &shv_path, const QDateTime &since, const QDateTime &until, QObject *parent = nullptr);

	void exec() override;

	const QDateTime &since() const { return m_since; }
	const QDateTime &until() const { return m_until; }

private:
	shv::core::utils::ShvGetLogParams logParams() const;
	void trimDirtyLog(const QDateTime &until);

	const QString m_shvPath;
	QDateTime m_since;
	QDateTime m_until;
	shv::core::utils::ShvMemoryJournal m_log;
	LogDir m_logDir;
};

#endif // DEVICELOGREQUEST_H
