#ifndef LOGSANITIZER_H
#define LOGSANITIZER_H

#include <QObject>
#include <QTimer>

#include "checklogtask.h"
#include <shv/iotqt/rpc/clientconnection.h>

class LogSanitizer : public QObject
{
	Q_OBJECT

public:
	explicit LogSanitizer(QObject *parent = nullptr);

private:
	void onShvStateChanged(shv::iotqt::rpc::ClientConnection::State state);
	void onDeviceAppeared(const QString &shv_path);
	void checkLogs();
	void checkLogs(const QString &shv_path, CheckLogType check_type);

	int m_lastCheckedDevice;
	QTimer m_timer;

	QStringList m_runningChecks;
};

#endif // LOGSANITIZER_H
