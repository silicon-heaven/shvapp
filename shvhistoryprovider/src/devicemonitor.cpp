#include "application.h"
#include "appclioptions.h"
#include "devicemonitor.h"
#include "shvsubscription.h"
#include "siteitem.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/rpc.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>

#include <QTimer>

namespace cp = shv::chainpack;

DeviceMonitor::DeviceMonitor(QObject *parent)
	: QObject(parent)
	, m_mntSubscription(nullptr)
	, m_sitesSubscription(nullptr)
	, m_sites(nullptr)
	, m_downloadingSites(false)
{
	connect(Application::instance()->deviceConnection(), &shv::iotqt::rpc::DeviceConnection::stateChanged, this, &DeviceMonitor::onShvStateChanged);
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
		QString shv_sites_path = QString::fromStdString(app->cliOptions()->shvSitesPath());
		QString path = "shv";
		if (!shv_sites_path.isEmpty()) {
			path += '/' + shv_sites_path;
		}

		m_mntSubscription = new ShvSubscription(conn, path, "mntchng", this);
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
	}
}

void DeviceMonitor::downloadSites()
{
	if (m_downloadingSites) {
		return;
	}
	m_downloadingSites = true;

	try {
		Application *app = Application::instance();
		auto *conn = app->deviceConnection();
		int rq_id = conn->nextRequestId();
		shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rq_id, this);
		cb->start([this](const cp::RpcResponse &response) {
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
				m_downloadingSites = false;
				scanDevices();
			}
			catch (const shv::core::Exception &e) {
				shvWarning() << "Error on parsing sites.json" << e.message();
				m_downloadingSites = false;
			}
		});
		conn->callShvMethod(rq_id, app->cliOptions()->sitesPath(), "getSites", cp::RpcValue());
	}
	catch (...) {
		m_downloadingSites = false;
		throw;
	}
}

void DeviceMonitor::onDeviceMountChanged(const QString &path, const QString &method, const shv::chainpack::RpcValue &data)
{
	Q_UNUSED(method);
	QString p = path.mid(4);  //remove shv/

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
	QStringList old_monitores_devices = m_monitoredDevices;
	for (const SitesHPDevice *device : m_sites->findChildren<SitesHPDevice*>()) {
		QString shv_path = device->shvPath();
		if (old_monitores_devices.removeOne(shv_path)) {
			continue;
		}
		m_monitoredDevices << shv_path;
		isDeviceOnline(shv_path, [this, shv_path](bool is_online) {
			if (is_online) {
				m_onlineDevices << shv_path;
				Q_EMIT deviceConnectedToBroker(shv_path);
			}
		});
	}
	for (const QString &shv_path : old_monitores_devices) {
		m_monitoredDevices.removeOne(shv_path);
		m_onlineDevices.removeOne(shv_path);
		Q_EMIT deviceRemovedFromSites(shv_path);
	}
	Q_EMIT sitesDownloadFinished();
}

void DeviceMonitor::isDeviceOnline(const QString &shv_path, std::function<void(bool)> callback)
{
	int slash = shv_path.lastIndexOf('/');
	QString parent_path = shv_path.mid(0, slash);
	std::string device_name = shv_path.mid(slash + 1).toStdString();

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
			if (list_item.isString() && list_item.toString() == device_name) {
				callback(true);
				return;
			}
		}
		callback(false);
	});
	conn->callShvMethod(rq_id, ("shv/" + parent_path).toStdString(), cp::Rpc::METH_LS, cp::RpcValue());
}
