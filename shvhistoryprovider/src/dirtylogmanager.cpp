#include "application.h"
#include "appclioptions.h"
#include "devicemonitor.h"
#include "dirtylogmanager.h"
#include "logdir.h"
#include "shvsubscription.h"

#include <shv/core/log.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/utils/shvlogfilereader.h>
#include <shv/iotqt/rpc/rpc.h>

using namespace shv::core::utils;

DirtyLogManager::DirtyLogManager(QObject *parent)
	: QObject(parent)
	, m_chngSubscription(nullptr)
	, m_cmdLogSubscription(nullptr)
{
	Application *app = Application::instance();
	auto *conn = app->deviceConnection();
	connect(conn, &shv::iotqt::rpc::DeviceConnection::stateChanged, this, &DirtyLogManager::onShvStateChanged);

	DeviceMonitor *monitor = app->deviceMonitor();

	insertDataMissingToDirtyLog();

	for (const QString &shv_path : monitor->onlineDevices()) {
		onDeviceAppeared(shv_path);
	}
	connect(monitor, &DeviceMonitor::deviceConnectedToBroker, this, &DirtyLogManager::onDeviceAppeared);
	connect(monitor, &DeviceMonitor::deviceDisconnectedFromBroker, this, &DirtyLogManager::onDeviceDisappeared);
	connect(monitor, &DeviceMonitor::deviceRemovedFromSites, this, &DirtyLogManager::onDeviceDisappeared);

	if (conn->state() == shv::iotqt::rpc::DeviceConnection::State::BrokerConnected) {
		onShvStateChanged(conn->state());
	}
}

DirtyLogManager::~DirtyLogManager()
{
	if (m_chngSubscription) {
		delete m_chngSubscription;
	}
	if (m_cmdLogSubscription) {
		delete m_cmdLogSubscription;
	}
}

void DirtyLogManager::onDeviceAppeared(const QString &shv_path)
{
	if (!Application::instance()->deviceMonitor()->isPushLogDevice(shv_path)) {
		writeDirtyLog(shv_path,
					  ShvJournalEntry::PATH_DATA_MISSING,
					  "",
					  QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(),
					  ShvJournalEntry::DOMAIN_SHV_SYSTEM,
					  true);
	}
}

void DirtyLogManager::onDeviceDisappeared(const QString &shv_path)
{
	if (!Application::instance()->deviceMonitor()->isPushLogDevice(shv_path)) {
		writeDirtyLog(shv_path,
					  ShvJournalEntry::PATH_DATA_MISSING,
					  ShvJournalEntry::DATA_MISSING_UNAVAILABLE,
					  QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(),
					  ShvJournalEntry::DOMAIN_SHV_SYSTEM,
					  false);
	}
}

void DirtyLogManager::onShvStateChanged(shv::iotqt::rpc::ClientConnection::State state)
{
	if (state == shv::iotqt::rpc::ClientConnection::State::BrokerConnected) {
		Application *app = Application::instance();
		auto *conn = app->deviceConnection();

		QString shv_sites_path = QString::fromStdString(app->cliOptions()->sitesRootPath());
		QString path = "shv";
		if (!shv_sites_path.isEmpty()) {
			path += '/' + shv_sites_path;
		}
		m_chngSubscription = new ShvSubscription(conn, path, shv::chainpack::Rpc::SIG_VAL_CHANGED, this);
		connect(m_chngSubscription, &ShvSubscription::notificationReceived, this, &DirtyLogManager::onDeviceDataChanged);
		m_cmdLogSubscription = new ShvSubscription(conn, path, shv::chainpack::Rpc::SIG_COMMAND_LOGGED, this);
		connect(m_cmdLogSubscription, &ShvSubscription::notificationReceived, this, &DirtyLogManager::onDeviceDataChanged);
	}
	else if (state == shv::iotqt::rpc::ClientConnection::State::NotConnected) {
		if (m_chngSubscription) {
			delete m_chngSubscription;
			m_chngSubscription = nullptr;
		}
		if (m_cmdLogSubscription) {
			delete m_cmdLogSubscription;
			m_cmdLogSubscription = nullptr;
		}
		insertDataMissingToDirtyLog();
	}
}

void DirtyLogManager::onDeviceDataChanged(const QString &path, const QString &method, const shv::chainpack::RpcValue &data)
{
	Q_UNUSED(method);
	Application *app = Application::instance();
	DeviceMonitor *dm = app->deviceMonitor();
	shv::chainpack::RpcValue value = data;

	QString p = path.mid(4);
	QString shv_path;
	QString property;
	for (const QString &device : dm->monitoredDevices()) {
		if (p.startsWith(device)) {
			shv_path = device;
			property = p.mid(device.length() + 1);
			break;
		}
	}
	if (!shv_path.isEmpty() && !dm->isPushLogDevice(shv_path)) {
		int64_t timestamp = 0;
		std::string domain = ShvJournalEntry::DOMAIN_VAL_CHANGE;
		if (shv::chainpack::DataChange::isDataChange(value)) {
			shv::chainpack::DataChange d = shv::chainpack::DataChange::fromRpcValue(value);
			if (d.hasDateTime()) {
				timestamp = d.epochMSec();
			}
			if (!d.domain().empty()) {
				domain = d.domain();
			}
			value = d.value();
		}
		if (!timestamp) {
			timestamp = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
		}

		writeDirtyLog(shv_path, property, value, timestamp, domain, true);
	}
}

void DirtyLogManager::insertDataMissingToDirtyLog()
{
	Application *app = Application::instance();
	DeviceMonitor *dm = app->deviceMonitor();
	const QStringList &monitored_devices = dm->monitoredDevices();
	for (const QString &shv_path : monitored_devices) {
		if (!dm->isPushLogDevice(shv_path)) {
			insertDataMissingToDirtyLog(shv_path);
		}
	}
}

void DirtyLogManager::insertDataMissingToDirtyLog(const QString &shv_path)
{
	checkDirtyLog(shv_path, false);
	LogDir log_dir(shv_path);
	ShvJournalFileReader dirty_reader(log_dir.dirtyLogPath().toStdString());
	ShvJournalEntry last_entry;
	if (dirty_reader.last()) {
		last_entry = dirty_reader.entry();
	}
	if (last_entry.path == ShvJournalEntry::PATH_DATA_MISSING &&
		last_entry.value.isString() &&
		last_entry.value.asString() == ShvJournalEntry::DATA_MISSING_UNAVAILABLE) {
		return;
	}
	ShvJournalFileWriter dirty_writer(log_dir.dirtyLogPath().toStdString());
	dirty_writer.append(ShvJournalEntry{
							ShvJournalEntry::PATH_DATA_MISSING,
							ShvJournalEntry::DATA_MISSING_UNAVAILABLE,
							ShvJournalEntry::DOMAIN_SHV_SYSTEM,
							ShvJournalEntry::NO_SHORT_TIME,
							ShvJournalEntry::SampleType::Continuous,
							last_entry.epochMsec
						});
}

void DirtyLogManager::writeDirtyLog(const QString &shv_path, const QString &path, const shv::chainpack::RpcValue &value, int64_t timestamp, std::string domain, bool is_connected)
{
	checkDirtyLog(shv_path, is_connected);
	LogDir log_dir(shv_path);
	ShvJournalFileWriter dirty_writer(log_dir.dirtyLogPath().toStdString());

	dirty_writer.append(ShvJournalEntry{
							path.toStdString(),
							value,
							domain,
							ShvJournalEntry::NO_SHORT_TIME,
							ShvJournalEntry::SampleType::Continuous,
							timestamp
						});
}

void DirtyLogManager::checkDirtyLog(const QString &shv_path, bool is_connected)
{
	LogDir log_dir(shv_path);
	if (!log_dir.exists(log_dir.dirtyLogName())) {
		QDateTime current = QDateTime::currentDateTimeUtc();
		QDateTime since = current;
		QStringList log_files = log_dir.findFiles(QDateTime(), QDateTime());
		if (log_files.count()) {
			ShvLogHeader latest_header = ShvLogFileReader(log_files.last().toStdString()).logHeader();
			since = rpcvalue_cast<QDateTime>(latest_header.until());
		}
		ShvJournalFileWriter dirty_writer(log_dir.dirtyLogPath().toStdString());
		dirty_writer.append(ShvJournalEntry{
								ShvJournalEntry::PATH_DATA_DIRTY,
								true,
								ShvJournalEntry::DOMAIN_SHV_SYSTEM,
								ShvJournalEntry::NO_SHORT_TIME,
								ShvJournalEntry::SampleType::Continuous,
								since.toMSecsSinceEpoch()
							});
		if (!is_connected || since < current) {
			dirty_writer.append(ShvJournalEntry{
									ShvJournalEntry::PATH_DATA_MISSING,
									ShvJournalEntry::DATA_MISSING_UNAVAILABLE,
									ShvJournalEntry::DOMAIN_SHV_SYSTEM,
									ShvJournalEntry::NO_SHORT_TIME,
									ShvJournalEntry::SampleType::Continuous,
									since.toMSecsSinceEpoch()
								});
			if (is_connected) {
				dirty_writer.append(ShvJournalEntry{
										ShvJournalEntry::PATH_DATA_MISSING,
										"",
										ShvJournalEntry::DOMAIN_SHV_SYSTEM,
										ShvJournalEntry::NO_SHORT_TIME,
										ShvJournalEntry::SampleType::Continuous,
										QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()
									});
			}
		}
		auto *conn = Application::instance()->deviceConnection();
		if (conn->state() == shv::iotqt::rpc::DeviceConnection::State::BrokerConnected) {
			conn->sendShvSignal((shv_path + "/" + Application::DIRTY_LOG_NODE + "/" + Application::START_TS_NODE).toStdString(),
								shv::chainpack::Rpc::SIG_VAL_CHANGED,
								shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(since.toMSecsSinceEpoch()));
		}
	}
}
