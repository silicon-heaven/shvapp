#ifndef CHECKLOGTASK_H
#define CHECKLOGTASK_H

#include "logdir.h"

#include <QVector>

class DeviceLogRequest;

class CheckLogTask : public QObject
{
	Q_OBJECT
	using Super = QObject;

public:
	enum class CheckType {
		ReplaceDirtyLog,       //on device apperance we need to remove dirty log as soon as possible, because it is inconsistent
		CheckDirtyLogState,    //periodic check, dirty log is replaced only if it is too big or too old
		TrimDirtyLogOnly,      //only trim dirty log if it is possible
	};

	CheckLogTask(const QString &shv_path, CheckType check_type, QObject *parent);

	void exec();
	Q_SIGNAL void finished(bool);
	CheckType checkType() const { return m_checkType; }

private:
	void checkOldDataConsistency();
	void checkDirtyLogState();
	void getLog(const QDateTime &since, const QDateTime &until);
	void execRequest(DeviceLogRequest *request);
	void checkRequestQueue();

	QString m_shvPath;
	CheckType m_checkType;
	LogDir m_logDir;
	QVector<DeviceLogRequest*> m_requests;
	QStringList m_dirEntries;
};

#endif // CHECKLOGTASK_H
