#ifndef CHECKLOGREQUEST_H
#define CHECKLOGREQUEST_H

#include "asyncrequest.h"
#include "logdir.h"

class DeviceLogRequest;

enum class CheckLogType {
	OnDeviceAppeared, //we need to remove dirty log, as soon as possible because it is inconsistent
	Periodic          //dirty log is replaced only if it is too big or too old
};

class CheckLogRequest : public AsyncRequest
{
	Q_OBJECT
	using Super = AsyncRequest;

public:
	CheckLogRequest(const QString &shv_path, CheckLogType check_type, QObject *parent);

	void exec() override;

private:
	void checkOldDataConsistency();
	void periodicDirtyLogCheck();
	void getLog(const QDateTime &since, const QDateTime &until);
	void execRequest(DeviceLogRequest *request);
	void checkRequestQueue();

	QString m_shvPath;
	CheckLogType m_checkType;
	LogDir m_logDir;
	QVector<DeviceLogRequest*> m_requests;
	QStringList m_dirEntries;
};

#endif // CHECKLOGREQUEST_H
