#ifndef DEVICEMONITOR_H
#define DEVICEMONITOR_H

#include <QObject>

#include <shv/chainpack/rpcvalue.h>
#include "abstractrequest.h"

class SiteItem;
class ShvSubscription;

class DeviceMonitor : public QObject
{
	Q_OBJECT

public:
	explicit DeviceMonitor(QObject *parent = nullptr);
	~DeviceMonitor();

	const QStringList &monitoredDevices() const;
	const QStringList &onlineDevices() const;
	bool isElesysDevice(const QString &shv_path) const;

	void downloadSites();
	const SiteItem *sites() const { return m_sites; }

	Q_SIGNAL void deviceAdded(const QString &device);
	Q_SIGNAL void deviceRemoved(const QString &device);
	Q_SIGNAL void deviceAppeared(const QString &device);
	Q_SIGNAL void deviceDisappeared(const QString &device);

	Q_SIGNAL void deviceScanFinished();

private:
	Q_SIGNAL void sitesDownloadFinished();

	void onShvStateChanged();
	void onDeviceMountChanged(const QString &path, const QString &method, const shv::chainpack::RpcValue &data);
	void onDeviceDataChanged(const QString &path, const QString &method, const shv::chainpack::RpcValue &data);

	void scanDevices();
	void isDeviceOnline(const QString &shv_path, BoolCallback callback);

	ShvSubscription *m_mntSubscription;
	ShvSubscription *m_sitesSubscription;
	QStringList m_monitoredDevices;
	QStringList m_onlineDevices;

	SiteItem *m_sites;
	bool m_downloadingSites;
};

#endif // DEVICEMONITOR_H
