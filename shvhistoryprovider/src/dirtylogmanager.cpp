#include "application.h"
#include "devicemonitor.h"
#include "dirtylogmanager.h"
#include "logdir.h"

#include <shv/core/log.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/utils/shvlogfilereader.h>
#include <shv/coreqt/rpc.h>

using namespace shv::core::utils;

DirtyLogManager::DirtyLogManager(QObject *parent)
	: QObject(parent)
{
	Application *app = Application::instance();
	auto *conn = app->deviceConnection();
	connect(conn, &shv::iotqt::rpc::DeviceConnection::stateChanged, this, &DirtyLogManager::onShvStateChanged);

	DeviceMonitor *monitor = app->deviceMonitor();

	insertDataMissingToDirtyLog();

	for (const QString &site_path : monitor->onlineDevices()) {
		onDeviceAppeared(site_path);
	}
	connect(monitor, &DeviceMonitor::deviceConnectedToBroker, this, &DirtyLogManager::onDeviceAppeared);
	connect(monitor, &DeviceMonitor::deviceDisconnectedFromBroker, this, &DirtyLogManager::onDeviceDisappeared);
	connect(monitor, &DeviceMonitor::deviceRemovedFromSites, this, &DirtyLogManager::onDeviceDisappeared);
	connect(app, &Application::deviceDataChanged, this, &DirtyLogManager::onDeviceDataChanged);

	if (conn->state() == shv::iotqt::rpc::DeviceConnection::State::BrokerConnected) {
		onShvStateChanged(conn->state());
	}
}

DirtyLogManager::~DirtyLogManager()
{
}

void DirtyLogManager::onDeviceAppeared(const QString &site_path)
{
	if (!Application::instance()->deviceMonitor()->isPushLogDevice(site_path)) {
		writeDirtyLog(site_path,
					  ShvJournalEntry::PATH_DATA_MISSING,
					  "",
					  QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(),
					  ShvJournalEntry::DOMAIN_SHV_SYSTEM,
					  true);
	}
}

void DirtyLogManager::onDeviceDisappeared(const QString &site_path)
{
	if (!Application::instance()->deviceMonitor()->isPushLogDevice(site_path)) {
		writeDirtyLog(site_path,
					  ShvJournalEntry::PATH_DATA_MISSING,
					  ShvJournalEntry::DATA_MISSING_UNAVAILABLE,
					  QDateTime::currentDateTimeUtc().toMSecsSinceEpoch(),
					  ShvJournalEntry::DOMAIN_SHV_SYSTEM,
					  false);
	}
}

void DirtyLogManager::onShvStateChanged(shv::iotqt::rpc::ClientConnection::State state)
{
	if (state == shv::iotqt::rpc::ClientConnection::State::NotConnected) {
		insertDataMissingToDirtyLog();
	}
}

void DirtyLogManager::onDeviceDataChanged(const QString &site_path, const QString &property, const QString &method, const shv::chainpack::RpcValue &data)
{
	Q_UNUSED(method);

	if (!Application::instance()->deviceMonitor()->isPushLogDevice(site_path)) {
		shv::chainpack::RpcValue value = data;
		if (ShvJournalEntry::isShvJournalEntry(value)) {
			writeDirtyLog(site_path, ShvJournalEntry::fromRpcValue(value), true);
		}
		else {
			int64_t timestamp = 0;
			std::string domain = ShvJournalEntry::DOMAIN_VAL_CHANGE;
			if (shv::chainpack::DataChange::isDataChange(value)) {
				shv::chainpack::DataChange d = shv::chainpack::DataChange::fromRpcValue(value);
				if (d.hasDateTime()) {
					timestamp = d.epochMSec();
				}
				value = d.value();
			}
			if (!timestamp) {
				timestamp = QDateTime::currentDateTimeUtc().toMSecsSinceEpoch();
			}
			writeDirtyLog(site_path, property, value, timestamp, domain, true);
		}
	}
}

void DirtyLogManager::insertDataMissingToDirtyLog()
{
	Application *app = Application::instance();
	DeviceMonitor *dm = app->deviceMonitor();
	const QStringList &monitored_devices = dm->monitoredDevices();
	for (const QString &site_path : monitored_devices) {
		if (!dm->isPushLogDevice(site_path)) {
			insertDataMissingToDirtyLog(site_path);
		}
	}
}

void DirtyLogManager::insertDataMissingToDirtyLog(const QString &site_path)
{
	checkDirtyLog(site_path, false);
	LogDir log_dir(site_path);
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
							ShvJournalEntry::NO_VALUE_FLAGS,
							last_entry.epochMsec
						});
}

void DirtyLogManager::writeDirtyLog(const QString &site_path, const QString &property, const shv::chainpack::RpcValue &value, int64_t timestamp, std::string domain, bool is_connected)
{
	writeDirtyLog(site_path,
				  ShvJournalEntry{
					  property.toStdString(),
					  value,
					  domain,
					  ShvJournalEntry::NO_SHORT_TIME,
					  ShvJournalEntry::NO_VALUE_FLAGS,
					  timestamp
				  }, is_connected);
}

void DirtyLogManager::writeDirtyLog(const QString &site_path, const ShvJournalEntry &entry, bool is_connected)
{
	checkDirtyLog(site_path, is_connected);
	LogDir log_dir(site_path);
	ShvJournalFileWriter dirty_writer(log_dir.dirtyLogPath().toStdString());
	dirty_writer.append(entry);
}

void DirtyLogManager::checkDirtyLog(const QString &site_path, bool is_connected)
{
	LogDir log_dir(site_path);
	if (!log_dir.existsDirtyLog()) {
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
								ShvJournalEntry::NO_VALUE_FLAGS,
								since.toMSecsSinceEpoch()
							});
		if (!is_connected || since < current) {
			dirty_writer.append(ShvJournalEntry{
									ShvJournalEntry::PATH_DATA_MISSING,
									ShvJournalEntry::DATA_MISSING_UNAVAILABLE,
									ShvJournalEntry::DOMAIN_SHV_SYSTEM,
									ShvJournalEntry::NO_SHORT_TIME,
									ShvJournalEntry::NO_VALUE_FLAGS,
									since.toMSecsSinceEpoch()
								});
			if (is_connected) {
				dirty_writer.append(ShvJournalEntry{
										ShvJournalEntry::PATH_DATA_MISSING,
										"",
										ShvJournalEntry::DOMAIN_SHV_SYSTEM,
										ShvJournalEntry::NO_SHORT_TIME,
										ShvJournalEntry::NO_VALUE_FLAGS,
										current.toMSecsSinceEpoch()
									});
			}
		}
		auto *conn = Application::instance()->deviceConnection();
		if (conn->state() == shv::iotqt::rpc::DeviceConnection::State::BrokerConnected) {
			conn->sendShvSignal((site_path + "/" + Application::DIRTY_LOG_NODE + "/" + Application::START_TS_NODE).toStdString(),
								shv::chainpack::Rpc::SIG_VAL_CHANGED,
								shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(since.toMSecsSinceEpoch()));
		}
	}
}
