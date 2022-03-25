#ifndef CHECKLOGTASK_H
#define CHECKLOGTASK_H

#include "logdir.h"
#include "datetimeinterval.h"
#include "checktype.h"

#include <QVector>

class DeviceLogRequest;
class DirConsistencyCheckTask;

enum class CacheStatus { OK, Warning, Error };

const char *cacheStatusToString(CacheStatus st);

class CacheError
{
	Q_GADGET

public:
	enum class Type
	{
		FirstFileMarkMissing,
		SinceUntilGap,
		SinceUntilOverlap,
		Fragmentation,
		DirtLogMissing,
		DirtyLogMarkMissing
	};

	Q_ENUM(Type)

	CacheStatus status;
	Type type;
	QString description;

	static const char *typeToString(Type t);
};

class CacheFileState
{
public:
	QString fileName;
	CacheStatus status;
	int recordCount = 0;
	QVector<CacheError> errors;
};

class CacheState
{
public:
	QDateTime since;
	QDateTime until;
	int recordCount = 0;
	int fileCount = 0;
	QVector<CacheFileState> files;
};

class CheckLogTask : public QObject
{
	Q_OBJECT
	using Super = QObject;

public:
	CheckLogTask(const QString &shv_path, CheckType check_type, QObject *parent);

	void exec();
	Q_SIGNAL void finished(bool);
	CheckType checkType() const { return m_checkType; }
	static CacheState checkLogCache(const QString &shv_path, bool with_good_files);

private:
	void onShvStateChanged();
	void onDeviceDisappeared(const QString &shv_path);
	void abort();
//	QVector<DateTimeInterval> checkDirConsistency();
	void onDirConsistencyChecked(const QVector<DateTimeInterval> &requested_intervals);
	void checkDirtyLogState();
	void getLog(const QDateTime &since, const QDateTime &until);
	void execRequest(DeviceLogRequest *request);
	void checkRequestQueue();

	QString m_shvPath;
	CheckType m_checkType;
	LogDir m_logDir;
	QVector<DeviceLogRequest*> m_requests;
	QStringList m_dirEntries;
	DirConsistencyCheckTask *m_dirConsistencyCheckTask = nullptr;
};

#endif // CHECKLOGTASK_H
