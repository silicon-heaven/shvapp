#include "devicemonitor.h"

#include "application.h"
#include "appclioptions.h"
#include "shvsubscription.h"
#include "siteitem.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/rpc.h>
#include <QSharedPointer>
#include <QTimer>

namespace cp = shv::chainpack;

DeviceMonitor::DeviceMonitor(QObject *parent)
	: QObject(parent)
	, m_mntSubscription(nullptr)
	, m_sitesSubscription(nullptr)
	, m_sites(nullptr)
	, m_downloadingSites(false)
{
	connect(Application::instance(), &Application::shvStateChanged, this, &DeviceMonitor::onShvStateChanged);
}

DeviceMonitor::~DeviceMonitor()
{
	if (m_mntSubscription) {
		delete m_mntSubscription;
	}
	if (m_sitesSubscription) {
		delete m_sitesSubscription;
	}
}

const QStringList &DeviceMonitor::monitoredDevices() const
{
	return m_monitoredDevices;
}

const QStringList &DeviceMonitor::onlineDevices() const
{
	return m_onlineDevices;
}

bool DeviceMonitor::isElesysDevice(const QString &shv_path) const
{
	const SitesHPDevice *hp_device = qobject_cast<const SitesHPDevice *>(m_sites->itemByShvPath(shv_path));
	return hp_device && hp_device->elesys();
}

void DeviceMonitor::onShvStateChanged()
{
	Application *app = Application::instance();
	auto *conn = app->deviceConnection();
	if (conn->state() == shv::iotqt::rpc::ClientConnection::State::BrokerConnected) {
		QString maintained_path = QString::fromStdString(app->cliOptions()->maintainedPath());
		QString path = "shv";
		if (!maintained_path.isEmpty()) {
			path += '/' + maintained_path;
		}

		m_mntSubscription = new ShvSubscription(conn, path, "mntchng", this);
		connect(m_mntSubscription, &ShvSubscription::notificationReceived, this, &DeviceMonitor::onDeviceMountChanged);
		conn->callMethodSubscribe(path.toStdString(), "mntchng");

		downloadSites();
		QString sites_path = QString::fromStdString(app->cliOptions()->masterBrokerPath()) + "sites";

		m_sitesSubscription = new ShvSubscription(conn, sites_path, "reloaded", this);
		connect(m_sitesSubscription, &ShvSubscription::notificationReceived, this, &DeviceMonitor::downloadSites);
		conn->callMethodSubscribe(sites_path.toStdString(), "reloaded");
	}
	else if (conn->state() == shv::iotqt::rpc::ClientConnection::State::NotConnected) {
		if (m_mntSubscription) {
			delete m_mntSubscription;
			m_mntSubscription = nullptr;
		}
		if (m_sitesSubscription) {
			delete  m_sitesSubscription;
			m_sitesSubscription = nullptr;
		}
	}
}

void DeviceMonitor::downloadSites()
{
	if (m_downloadingSites) {
		return;
	}
	m_downloadingSites = true;

	Application *app = Application::instance();

	app->shvCall(QString::fromStdString(app->cliOptions()->masterBrokerPath()) + "sites", "getSites", [this](const shv::chainpack::RpcResponse &response) {
		if (response.isError()) {
			shvError() << response.error().message();
			QTimer::singleShot(60 * 1000, this, &DeviceMonitor::downloadSites);
			m_downloadingSites = false;
			return;
		}
		shvInfo() << "sites loaded";
		QTimer::singleShot(60 * 60 * 1000, this, &DeviceMonitor::downloadSites);
		try {
			SiteItem *new_sites = new SiteItem(this);
			new_sites->parseRpcValue(response.result());
			if (m_sites) {
				delete m_sites;
			}
			m_sites = new_sites;
		}
		catch (const shv::core::Exception &e) {
			shvWarning() << "Error on parsing sites.json" << e.message();
		}
		m_downloadingSites = false;
		scanDevices();
	});
}

void DeviceMonitor::onDeviceMountChanged(const QString &path, const QString &method, const shv::chainpack::RpcValue &data)
{
	Q_UNUSED(method);
	QString p = path.mid(4);  //remove shv/
	shvInfo() << "device mount changed" << path << p;

//	if (!Application::instance()->cliOptions()->test() && p.startsWith("test")) {
//		return;
//	}

	bool mounted = data.toBool();
	if (mounted) {
		for (const QString &shv_path : m_monitoredDevices) {
			if (shv_path.startsWith(p)) {
				shvInfo() << "device" << shv_path << "appeared";
				m_onlineDevices << shv_path;
				Q_EMIT deviceConnectedToBroker(shv_path);
			}
		}
	}
	else {
		for (const QString &shv_path : m_monitoredDevices) {
			if (shv_path.startsWith(p)) {
				shvInfo() << "device" << shv_path << "disappeared";
				m_onlineDevices.removeOne(shv_path);
				Q_EMIT deviceDisconnectedFromBroker(shv_path);
			}
		}
	}
}

void DeviceMonitor::scanDevices()
{
	QString maintained_path = QString::fromStdString(Application::instance()->cliOptions()->maintainedPath());
	QStringList old_monitores_devices = m_monitoredDevices;
	//QSharedPointer<int> online_test_cnt(new int(1));
	for (const SitesHPDevice *device : m_sites->findChildren<SitesHPDevice*>()) {
		QString shv_path = device->shvPath();
		if (shv_path.startsWith(maintained_path)) {
//			if (Application::instance()->cliOptions()->test() || !shv_path.startsWith("test")) {
			if (old_monitores_devices.removeOne(shv_path)) {
				continue;
			}
			m_monitoredDevices << shv_path;
//			checkLogs(shv_path);
			//Q_EMIT deviceAddedToSites(shv_path);
			isDeviceOnline(shv_path, [this, shv_path](bool is_online) {
				if (is_online) {
					m_onlineDevices << shv_path;
					Q_EMIT deviceConnectedToBroker(shv_path);
				}
			});
//			}
		}
	}
	for (const QString &shv_path : old_monitores_devices) {
		m_monitoredDevices.removeOne(shv_path);
		m_onlineDevices.removeOne(shv_path);
		//if (m_onlineDevices.contains(shv_path)) {
		//	Q_EMIT deviceDisconnectedFromBroker(shv_path);
		//}
		Q_EMIT deviceRemovedFromSites(shv_path);
	}
	Q_EMIT sitesDownloadFinished();
}

void DeviceMonitor::isDeviceOnline(const QString &shv_path, BoolCallback callback)
{
	Application::instance()->shvCall("shv/" + shv_path, cp::Rpc::METH_DIR, [callback](const shv::chainpack::RpcResponse &response) {
		callback(!response.isError());
	});
}
