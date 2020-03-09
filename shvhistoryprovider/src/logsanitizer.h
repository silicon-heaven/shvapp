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

private:
	void onShvStateChanged(shv::iotqt::rpc::ClientConnection::State state);
	void onDeviceAppeared(const QString &shv_path);
	void checkLogs();
	void checkLogs(const QString &shv_path, CheckLogTask::CheckType check_type);

	int m_lastCheckedDevice;
	QTimer m_timer;
};

#endif // LOGSANITIZER_H
