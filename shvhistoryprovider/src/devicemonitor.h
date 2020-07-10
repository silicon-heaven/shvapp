#ifndef DEVICEMONITOR_H
#define DEVICEMONITOR_H

#include <shv/chainpack/rpcvalue.h>

#include <QObject>
#include <QTimer>
#include <functional>

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
	bool isPushLogDevice(const QString &shv_path) const;

	void downloadSites();
	const SiteItem *sites() const { return m_sites; }

	Q_SIGNAL void deviceRemovedFromSites(const QString &device);
	Q_SIGNAL void deviceConnectedToBroker(const QString &device);
	Q_SIGNAL void deviceDisconnectedFromBroker(const QString &device);

	Q_SIGNAL void sitesDownloadFinished();
private:

	void onShvStateChanged();
	void onDeviceMountChanged(const QString &path, const QString &method, const shv::chainpack::RpcValue &data);
	void onDeviceDataChanged(const QString &path, const QString &method, const shv::chainpack::RpcValue &data);

	void scanDevices();
	void isDeviceOnline(const QString &shv_path, std::function<void(bool)> callback);

	ShvSubscription *m_mntSubscription;
	ShvSubscription *m_sitesSubscription;
	QStringList m_monitoredDevices;
	QStringList m_onlineDevices;

	SiteItem *m_sites;
	QTimer m_downloadSitesTimer;
};

#endif // DEVICEMONITOR_H
