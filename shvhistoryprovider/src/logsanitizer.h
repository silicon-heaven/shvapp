#ifndef LOGSANITIZER_H
#define LOGSANITIZER_H

#include "checklogtask.h"
#include <shv/iotqt/rpc/clientconnection.h>

#include <QMap>
#include <QObject>
#include <QTimer>

class LogSanitizer : public QObject
{
	Q_OBJECT

public:
	explicit LogSanitizer(QObject *parent = nullptr);

	void trimDirtyLog(const QString &shv_path);
	bool sanitizeLogCache(const QString &shv_path, CheckLogTask::CheckType check_type);

private:
	void onShvStateChanged();
	void onDeviceAppeared(const QString &shv_path);
	void onDeviceDisappeared(const QString &shv_path);
	void sanitizeLogCache();
	void setupTimer();

	int m_lastCheckedDevice;
	QTimer m_timer;
	int m_interval = 0;
	QMap<QString, QTimer*> m_newDeviceTimers;
};

#endif // LOGSANITIZER_H
