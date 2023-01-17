#include "application.h"
#include "appclioptions.h"
#include "devicemonitor.h"
#include "shvsubscription.h"
#include "siteitem.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>

#include <QTimer>

namespace cp = shv::chainpack;

DeviceMonitor::DeviceMonitor(QObject *parent)
	: QObject(parent)
	, m_mntSubscription(nullptr)
	, m_sitesSubscription(nullptr)
	, m_sites(nullptr)
	, m_downloadSitesTimer(this)
{
	connect(Application::instance()->deviceConnection(), &shv::iotqt::rpc::DeviceConnection::stateChanged, this, &DeviceMonitor::onShvStateChanged);
	m_downloadSitesTimer.setSingleShot(true);
	connect(&m_downloadSitesTimer, &QTimer::timeout, this, &DeviceMonitor::downloadSites);
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

bool DeviceMonitor::isElesysDevice(const QString &site_path) const
{
	const SitesHPDevice *hp_device = qobject_cast<const SitesHPDevice *>(m_sites->itemBySitePath(site_path));
	return hp_device && hp_device->isElesys();
}

bool DeviceMonitor::isPushLogDevice(const QString &site_path) const
{
	const SitesHPDevice *hp_device = qobject_cast<const SitesHPDevice *>(m_sites->itemBySitePath(site_path));
	return hp_device && hp_device->isPushLog();
}

QString DeviceMonitor::syncLogBroker(const QString &site_path) const
{
	const SitesHPDevice *hp_device = qobject_cast<const SitesHPDevice *>(m_sites->itemBySitePath(site_path));
	if (hp_device && hp_device->isPushLog()) {
		return hp_device->syncLogSource();
	}
	return QString();
}


void DeviceMonitor::onShvStateChanged()
{
	Application *app = Application::instance();
	auto *conn = app->deviceConnection();
	if (conn->state() == shv::iotqt::rpc::ClientConnection::State::BrokerConnected) {
		QString shv_sites_path = QString::fromStdString(app->cliOptions()->sitesRootPath());
		QString path = "shv";
		if (!shv_sites_path.isEmpty()) {
			path += '/' + shv_sites_path;
		}

		m_mntSubscription = new ShvSubscription(conn, path, cp::Rpc::SIG_MOUNTED_CHANGED, this);
		connect(m_mntSubscription, &ShvSubscription::notificationReceived, this, &DeviceMonitor::onDeviceMountChanged);

		downloadSites();
		m_sitesSubscription = new ShvSubscription(conn, QString::fromStdString(app->cliOptions()->sitesPath()), "reloaded", this);
		connect(m_sitesSubscription, &ShvSubscription::notificationReceived, this, &DeviceMonitor::downloadSites);
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
		m_monitoredDevices.clear();
		m_onlineDevices.clear();
	}
}

void DeviceMonitor::downloadSites()
{
	if (m_downloadSitesTimer.isActive()) {
		m_downloadSitesTimer.start(1);
		return;
	}

	try {
		Application *app = Application::instance();
		auto *conn = app->deviceConnection();
		int rq_id = conn->nextRequestId();
		shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rq_id, this);
		cb->start([this](const cp::RpcResponse &response) {
			if (response.isError()) {
				shvError() << response.error().message();
				m_downloadSitesTimer.start(60 * 1000);
				return;
			}
			shvInfo() << "sites loaded";
			m_downloadSitesTimer.start(60 * 60 * 1000);
			try {
				SiteItem *new_sites = new SiteItem(this);
				new_sites->parseRpcValue(response.result());
				if (m_sites) {
					delete m_sites;
				}
				m_sites = new_sites;
				scanDevices();
			}
			catch (const shv::core::Exception &e) {
				shvWarning() << "Error on parsing sites.json" << e.message();
			}
		});
		conn->callShvMethod(rq_id, app->cliOptions()->sitesPath(), "getSites", cp::RpcValue());
	}
	catch (...) {
		m_downloadSitesTimer.start(60 * 1000);
		throw;
	}
}

void DeviceMonitor::onDeviceMountChanged(const QString &shv_path, const QString &method, const shv::chainpack::RpcValue &data)
{
	Q_UNUSED(method);
	QString mounted_site_path = shv_path.mid(4);  //remove shv/

	bool mounted = data.toBool();
	if (mounted) {
		for (const QString &site_path : m_monitoredDevices) {
			if (site_path.startsWith(mounted_site_path)) {
				isDeviceOnline(site_path, [this, site_path](bool is_online) {
					if (is_online) {
						shvInfo() << "device" << site_path << "connected";
						m_onlineDevices << site_path;
						Q_EMIT deviceConnectedToBroker(site_path);
					}
				});
			}
		}
	}
	else {
		for (const QString &site_path : m_monitoredDevices) {
			if (site_path.startsWith(mounted_site_path)) {
				if (m_onlineDevices.removeOne(site_path)) {
					shvInfo() << "device" << site_path << "disconnected";
					Q_EMIT deviceDisconnectedFromBroker(site_path);
				}
			}
		}
	}
}

void DeviceMonitor::scanDevices()
{
	QStringList old_monitores_devices = m_monitoredDevices;
	for (const SitesHPDevice *device : m_sites->findChildren<SitesHPDevice*>()) {
		QString site_path = device->sitePath();
		if (old_monitores_devices.removeOne(site_path)) {
			continue;
		}
		m_monitoredDevices << site_path;
		isDeviceOnline(site_path, [this, site_path](bool is_online) {
			if (is_online) {
				m_onlineDevices << site_path;
				Q_EMIT deviceConnectedToBroker(site_path);
			}
		});
	}
	for (const QString &site_path : old_monitores_devices) {
		m_monitoredDevices.removeOne(site_path);
		m_onlineDevices.removeOne(site_path);
		Q_EMIT deviceRemovedFromSites(site_path);
	}
	Q_EMIT sitesDownloadFinished();
}

void DeviceMonitor::isDeviceOnline(const QString &site_path, std::function<void(bool)> callback)
{
	int slash = static_cast<int>(site_path.lastIndexOf('/'));
	QString parent_path = site_path.mid(0, slash == -1 ? 0 : slash);
	std::string device_name = site_path.mid(slash + 1).toStdString();

	auto *conn = Application::instance()->deviceConnection();
	int rq_id = conn->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rq_id, this);
	cb->start([callback, device_name](const cp::RpcResponse &response) {
		if (response.isError()) {
			callback(false);
		}
		const cp::RpcValue &result = response.result();
		if (!result.isList()) {
			callback(false);
		}
		const cp::RpcValue::List &result_list = result.toList();
		for (const cp::RpcValue &list_item : result_list) {
			if (list_item.isString() && list_item.asString() == device_name) {
				callback(true);
				return;
			}
		}
		callback(false);
	});
	conn->callShvMethod(rq_id, ("shv/" + parent_path).toStdString(), cp::Rpc::METH_LS, cp::RpcValue());
}
