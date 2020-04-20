#ifndef LOGSANITIZER_H
#define LOGSANITIZER_H

#include "checklogtask.h"
#include <shv/iotqt/rpc/clientconnection.h>

#include <QObject>
#include <QTimer>

class LogSanitizer : public QObject
{
	Q_OBJECT

public:
	explicit LogSanitizer(QObject *parent = nullptr);

	void trimDirtyLog(const QString &shv_path);
	bool checkLogs(const QString &shv_path, CheckLogTask::CheckType check_type);

private:
	void onDeviceAppeared(const QString &shv_path);
	void checkLogs();
	void setupTimer();

	int m_lastCheckedDevice;
	QTimer m_timer;
};

#endif // LOGSANITIZER_H
