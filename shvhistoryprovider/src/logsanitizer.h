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

	void trimDirtyLog(const QString &site_path);
	bool sanitizeLogCache(const QString &site_path, CheckType check_type);

private:
	void onShvStateChanged();
	void onDeviceAppeared(const QString &site_path);
	void onDeviceDisappeared(const QString &site_path);
	void sanitizeLogCache();
	void setupTimer();
	void checkNewDevicesQueue();
	void planDirtyLogTrim(const QString &site_path);
	void onDeviceDataChanged(const QString &site_path, const QString &property, const QString &method, const shv::chainpack::RpcValue &data);

	int m_lastCheckedDevice;
	QTimer m_timer;
	int m_interval = 0;
	QMap<QString, QTimer*> m_newDeviceTimers;
	QStringList m_newDevices;
};

#endif // LOGSANITIZER_H
