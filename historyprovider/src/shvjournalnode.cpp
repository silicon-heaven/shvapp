#include "shvjournalnode.h"
#include "historyapp.h"
#include "appclioptions.h"
#include "utils.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpccall.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/utils/shvjournalentry.h>
#include <shv/core/utils/shvfilejournal.h>
#include <shv/core/utils/shvmemoryjournal.h>

#include <QDir>
#include <QDirIterator>

#include <QScopeGuard>

#include <QCoroSignal>

#define journalDebug() shvCDebug("historyjournal")
#define journalInfo() shvCInfo("historyjournal")
#define journalError() shvCError("historyjournal")

namespace cp = shv::chainpack;
namespace {
static std::vector<cp::MetaMethod> methods {
	{"getLog", cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{"syncLog", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_WRITE},
};
}

ShvJournalNode::ShvJournalNode(const std::vector<SlaveHpInfo>& slave_hps, ShvNode* parent)
	: Super(QString::fromStdString(HistoryApp::instance()->cliOptions()->journalCacheRoot()), "shvjournal", parent)
	, m_slaveHps(slave_hps)
	, m_cacheDirPath(QString::fromStdString(HistoryApp::instance()->cliOptions()->journalCacheRoot()))
{
	QDir(m_cacheDirPath).mkpath(".");
	auto conn = HistoryApp::instance()->rpcConnection();

	connect(conn, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvJournalNode::onRpcMessageReceived);

	for (const auto& it : slave_hps) {
		conn->callMethodSubscribe(it.shv_path, "mntchng");

		if (it.log_type != LogType::PushLog) {
			conn->callMethodSubscribe(it.shv_path, "chng");
		}

		auto tmr = new QTimer(this);
		connect(tmr, &QTimer::timeout, [path_to_sync = it.shv_path] { HistoryApp::instance()->shvJournalNode()->syncLog(path_to_sync, [] (auto /*error*/) { }); });
		tmr->start(HistoryApp::instance()->cliOptions()->logMaxAge() * 1000);
	}
}

namespace {
auto get_cache_dir_path(const QString& cache_root, const QString& slave_hp_path)
{
	using shv::coreqt::Utils;
	return Utils::joinPath(Utils::joinPath(cache_root, slave_hp_path), "_shvjournal");
}

const auto DIRTY_FILENAME = "dirty.log2";

std::string dirty_log_path(const QString& cache_dir_path)
{
	return QDir(cache_dir_path).filePath(DIRTY_FILENAME).toStdString();
}
}

void ShvJournalNode::onRpcMessageReceived(const cp::RpcMessage &msg)
{
	if (msg.isSignal()) {
		cp::RpcSignal ntf(msg);
		auto path = ntf.shvPath().asString();
		auto method = ntf.method().asString();
		auto params = ntf.params();
		auto value = ntf.value();

		auto it = std::find_if(m_slaveHps.begin(), m_slaveHps.end(), [&path] (const auto& slave_hp) {
			return path.starts_with(slave_hp.shv_path);
		});

		if (it != m_slaveHps.end()) {
			if (method == "chng") {
				{
					auto writer = shv::core::utils::ShvJournalFileWriter(dirty_log_path(get_cache_dir_path(m_cacheDirPath, QString::fromStdString(it->shv_path))));
					auto path_without_prefix = path.substr(it->shv_path.size());
					auto data_change = shv::chainpack::DataChange::fromRpcValue(ntf.params());

					auto entry = shv::core::utils::ShvJournalEntry(path_without_prefix, data_change.value()
															, shv::core::utils::ShvJournalEntry::DOMAIN_VAL_CHANGE
															, shv::core::utils::ShvJournalEntry::NO_SHORT_TIME
															, shv::core::utils::ShvJournalEntry::NO_VALUE_FLAGS
															, data_change.epochMSec());
					writer.append(entry);

				}
			}

			if (!m_syncInProgress.value(QString::fromStdString(it->shv_path), false) && method == "mntchng" && params.toBool() == true) {
				journalInfo() << "mntchng on" << it->shv_path << "syncing its journal";
				syncLog(it->shv_path, [] (auto /*error*/) {
				});
			}
		}
	}
}

size_t ShvJournalNode::methodCount(const StringViewList& shv_path)
{
	if (!shv_path.empty()) {
		// We'll let LocalFSNode handle the child nodes.
		return Super::methodCount(shv_path);
	}

	return Super::methodCount(shv_path) + methods.size();
}

const cp::MetaMethod* ShvJournalNode::metaMethod(const StringViewList& shv_path, size_t index)
{
	if (!shv_path.empty()) {
		// We'll let LocalFSNode handle the child nodes.
		return Super::metaMethod(shv_path, index);
	}

	static auto super_method_count = Super::methodCount(shv_path);

	if (index >= super_method_count) {
		return &methods.at(index - super_method_count);
	}

	return Super::metaMethod(shv_path, index);
}

namespace {

void write_entries_to_file(const QString& file_path, const std::vector<shv::core::utils::ShvJournalEntry>& entries)
{
	journalDebug() << "Writing" << entries.size() << "entries to" << file_path;
	QFile(file_path).remove();
	shv::core::utils::ShvJournalFileWriter writer(file_path.toStdString());
	for (const auto& entry : entries) {
		writer.append(entry);
	}
}
}

void ShvJournalNode::trimDirtyLog(const QString& slave_hp_path)
{
	journalInfo() << "Trimming dirty log for" << slave_hp_path;
	using shv::coreqt::Utils;
	auto cache_dir_path = get_cache_dir_path(m_cacheDirPath, slave_hp_path);
	QDir cache_dir(cache_dir_path);
	auto entries = cache_dir.entryList(QDir::NoDotAndDotDot | QDir::Files, QDir::Name | QDir::Reversed);

	if (entries.empty() || entries.at(0) != DIRTY_FILENAME) {
		journalDebug() << "No dirty log, nothing to trim";
		return;
	}

	if (entries.size() == 1) {
		journalDebug() << "No logs to use for trimming";
		return;
	}

	if (!shv::core::utils::ShvJournalFileReader(dirty_log_path(cache_dir_path)).next()) {
		// No entries in the dirty log file, nothing to change.
		journalDebug() << "Dirty log empty, nothing to trim";
		return;
	}

	journalDebug() << "File used for trimming" << cache_dir.filePath(entries.at(1));

	auto newest_file_entries = read_entries_from_file(cache_dir.filePath(entries.at(1)));
	int64_t newest_entry_msec = 0;
	if (!newest_file_entries.empty()) {
		newest_entry_msec = newest_file_entries.back().epochMsec;
	}
	// It is possible that the newest logfile contains multiple entries with the same timestamp. Because it could
	// have been written (by shvagent) in between two events that happenede in the same millisecond, we can't be
	// sure whether we have all events from the last millisecond. Because of that we will discard the last
	// millisecond from the synced log, and keep it in the dirty log.
	newest_file_entries.erase(std::find_if(newest_file_entries.begin(), newest_file_entries.end(), [newest_entry_msec] (const auto& entry) {
		return entry.epochMsec == newest_entry_msec;
	}), newest_file_entries.end());

	// If we discarded all the entries from the newest file (i.e. it only
	// contained data from the same timestamp), we'll just delete it. No
	// point in having empty files. After that we don't have to do anything
	// to the dirty log, because there's no data to trim.

	if (newest_file_entries.empty()) {
		QFile(cache_dir.filePath(entries.at(1))).remove();
		return;
	}

	write_entries_to_file(cache_dir.filePath(entries.at(1)), newest_file_entries);

	// Now filter dirty log's newer events.
	shv::core::utils::ShvJournalFileReader reader(dirty_log_path(cache_dir_path));
	std::vector<shv::core::utils::ShvJournalEntry> new_dirty_log_entries;
	while (reader.next()) {
		if (reader.entry().epochMsec >= newest_entry_msec) {
			new_dirty_log_entries.push_back(reader.entry());
		}
	}

	write_entries_to_file(QString::fromStdString(dirty_log_path(cache_dir_path)), new_dirty_log_entries);
}

namespace {
const auto RECORD_COUNT_LIMIT = 5000;
const auto MAX_ENTRIES_PER_FILE = 50000;
const auto SYNCER_MEMORY_LIMIT = 1024 /*B*/ * 1024 /*kB*/ * 100 /*MB*/;

const auto LS_FILES_RESPONSE_FILENAME = 0;
const auto LS_FILES_RESPONSE_FILETYPE = 1;
const auto LS_FILES_RESPONSE_FILESIZE = 2;
}

class FileSyncer : public QObject {
	Q_OBJECT
public:
	FileSyncer(ShvJournalNode* node, std::vector<SlaveHpInfo> slave_hps, const std::string& shv_path, const QString& cache_dir_path, const std::function<void(cp::RpcResponse::Error)>& callback, QMap<QString, bool>& syncInProgress)
		: m_node(node)
		, m_slaveHps(slave_hps)
		, m_shvPath(QString::fromStdString(shv_path))
		, m_cacheDirPath(cache_dir_path)
		, m_callback(callback)
		, m_syncInProgress(syncInProgress)
	{
		syncFiles();
	}

	void writeFiles()
	{
		QMapIterator iter(m_downloadedFiles);
		while (iter.hasNext()) {
			iter.next();
			QFile log_file(iter.key());
			if (log_file.open(QFile::WriteOnly | QFile::Append)) {
				auto blob = iter.value().asBlob();
				journalDebug() << "Writing" << iter.key();
				log_file.write(reinterpret_cast<const char*>(blob.data()), blob.size());
			} else {
				journalError() << "Couldn't open" << iter.key() << "for writing";
			}

		}

		m_downloadedFiles.clear();
		current_memory_usage = 0;
	}

	enum class SyncType {
		Device,
		Slave
	};

	void writeEntriesToFile(const std::vector<shv::core::utils::ShvJournalEntry>& downloaded_entries, const QString& slave_hp_path)
	{
		journalInfo() << "Writing" << downloaded_entries.size() << "entries for" << slave_hp_path;
		if (downloaded_entries.empty()) {
			return;
		}

		QDir dir(get_cache_dir_path(m_cacheDirPath, slave_hp_path));

		auto file_name = dir.filePath(QString::fromStdString(shv::core::utils::ShvJournalFileReader::msecToBaseFileName(downloaded_entries.front().epochMsec)) + ".log2");

		write_entries_to_file(file_name, downloaded_entries);
		m_node->trimDirtyLog(slave_hp_path);
	}

	QCoro::Task<void, QCoro::TaskOptions<QCoro::Options::AbortOnException>> doLegacySync(const QString& slave_hp_path, const SyncType sync_type)
	{
		Q_UNUSED(sync_type)
		journalInfo() << "Syncing" << slave_hp_path << "via legacy getLog";
		using shv::coreqt::Utils;
		shv::core::utils::ShvGetLogParams get_log_params;
		get_log_params.recordCountLimit = RECORD_COUNT_LIMIT;
		get_log_params.until = shv::chainpack::RpcValue::DateTime::now();
		get_log_params.since = shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(QDateTime::currentDateTime().addSecs(- HistoryApp::instance()->cliOptions()->cacheInitMaxAge()).toMSecsSinceEpoch());

		QDir cache_dir(get_cache_dir_path(m_cacheDirPath, slave_hp_path));
		if (!cache_dir.isEmpty()) {
			auto entry_list = cache_dir.entryList(QDir::NoDotAndDotDot | QDir::Files, QDir::Name | QDir::Reversed);
			QString newest_file_name;

			if (entry_list.at(0) != DIRTY_FILENAME) {
				newest_file_name = entry_list.at(0);
			} else if (entry_list.size() > 1) {
				newest_file_name = entry_list.at(1);
			}

			if (!newest_file_name.isEmpty()) {
				auto newest_file_entries = read_entries_from_file(cache_dir.filePath(newest_file_name));
				if (!newest_file_entries.empty()) {
					get_log_params.since = shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(newest_file_entries.back().dateTime().msecsSinceEpoch() + 1);
					journalDebug() << "Newest entry" << get_log_params.since << "for" << slave_hp_path;
				}
			}
		}

		std::vector<shv::core::utils::ShvJournalEntry> downloaded_entries;
		QScopeGuard entry_writer_guard([this, &slave_hp_path, &downloaded_entries] {
			writeEntriesToFile(downloaded_entries, slave_hp_path);
		});

		while (true) {
			auto call = shv::iotqt::rpc::RpcCall::create(HistoryApp::instance()->rpcConnection())
				->setShvPath(slave_hp_path)
				->setMethod("getLog")
				->setParams(get_log_params.toRpcValue());
			call->start();
			auto [result, error] = co_await qCoro(call, &shv::iotqt::rpc::RpcCall::maybeResult);

			if (!error.isEmpty()) {
				journalError() << "Error retrieving logs via getLog for:" << slave_hp_path << error;
				m_callback(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, "Couldn't retrieve all logs from the device"));
				co_return;
			}

			shv::core::utils::ShvMemoryJournal result_log;
			result_log.loadLog(result);
			result_log.clearSnapshot();
			if (result_log.isEmpty()) {
				co_return;
			}

			const auto& remote_entries = result_log.entries();

			journalInfo() << "Loaded" << remote_entries.size() << "log entries for" << slave_hp_path;
			auto newest_entry = remote_entries.back().epochMsec;
			for (const auto& entry : remote_entries) {
				// We're skipping all entries from the last millisecond, and retrieve it again, otherwise we can't be
				// sure if we got all from the same millisecond.
				if (entry.epochMsec == newest_entry) {
					break;
				}
				downloaded_entries.push_back(entry);
			}

			if (remote_entries.size() < RECORD_COUNT_LIMIT) {
				co_return;
			}

			if (downloaded_entries.size() > MAX_ENTRIES_PER_FILE) {
				writeEntriesToFile(downloaded_entries, slave_hp_path);
				downloaded_entries.clear();
			}
			get_log_params.since = remote_entries.back().dateTime();
		}
	}

	QCoro::Task<void, QCoro::TaskOptions<QCoro::Options::AbortOnException>> doSync(const QString& slave_hp_path, const SyncType sync_type)
	{
		journalInfo() << "Syncing" << slave_hp_path << "via file synchronization";
		using shv::coreqt::Utils;
		auto shvjournal_suffix =
			sync_type == SyncType::Device ?
			".app/shvjournal" :
			Utils::joinPath(".local/history/shvjournal", m_shvPath.remove(0, slave_hp_path.size()));
		auto shvjournal_shvpath = Utils::joinPath(slave_hp_path, shvjournal_suffix);

		auto call = shv::iotqt::rpc::RpcCall::create(HistoryApp::instance()->rpcConnection())
			->setShvPath(shvjournal_shvpath)
			->setMethod("lsfiles")
			->setParams(cp::RpcValue::Map{{"size", true}});
		call->start();

		auto [file_list, error] = co_await qCoro(call, &shv::iotqt::rpc::RpcCall::maybeResult);
		if (!error.isEmpty()) {
			journalError() << "Couldn't retrieve filelist from:" << shvjournal_shvpath << error;
			m_callback(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, "Couldn't retrieve filelist from the device"));
			co_return;
		}

		journalDebug() << "Got filelist from" << slave_hp_path;

		QDir cache_dir(get_cache_dir_path(m_cacheDirPath, slave_hp_path));
		QString newest_file_name;
		int64_t newest_file_name_ms = QDateTime::currentDateTime().addSecs(- HistoryApp::instance()->cliOptions()->cacheInitMaxAge()).toMSecsSinceEpoch();
		auto entry_list = cache_dir.entryList(QDir::NoDotAndDotDot | QDir::Files, QDir::Name | QDir::Reversed);
		if (!entry_list.empty()) {
			if (entry_list.at(0) != DIRTY_FILENAME) {
				newest_file_name = entry_list.at(0);
				newest_file_name_ms = shv::core::utils::ShvJournalFileReader::fileNameToFileMsec(newest_file_name.toStdString());
			} else if (entry_list.size() > 1) {
				newest_file_name = entry_list.at(1);
				newest_file_name_ms = shv::core::utils::ShvJournalFileReader::fileNameToFileMsec(newest_file_name.toStdString());
			}
		}

		journalDebug() << "Newest file for" << slave_hp_path << "is" << newest_file_name;

		for (const auto& current_file : file_list.asList()) {
			if (current_memory_usage > SYNCER_MEMORY_LIMIT) {
				writeFiles();
			};
			auto file_name = QString::fromStdString(current_file.asList().at(LS_FILES_RESPONSE_FILENAME).asString());
			if (file_name == "dirty.log2") {
				continue;
			}

			if (shv::core::utils::ShvJournalFileReader::fileNameToFileMsec(file_name.toStdString()) < newest_file_name_ms) {
				journalDebug() << "Skipping" << file_name << "because it's older than our newest file" << newest_file_name;
				continue;
			}

			auto full_file_name = QDir(get_cache_dir_path(m_cacheDirPath, slave_hp_path)).filePath(file_name);
			QFile file(full_file_name);
			auto local_size = file.size();
			auto remote_size = current_file.asList().at(LS_FILES_RESPONSE_FILESIZE).toInt();

			journalInfo() << "Syncing file" << full_file_name << "remote size:" << remote_size << "local size:" << (file.exists() ? QString::number(local_size) : "<doesn't exist>");
			if (file.exists()) {
				if (local_size == remote_size) {
					continue;
				}
			}

			auto sites_log_file = Utils::joinPath(shvjournal_shvpath, file_name);
			journalDebug() << "Retrieving" << sites_log_file << "offset:" << local_size;
			call = shv::iotqt::rpc::RpcCall::create(HistoryApp::instance()->rpcConnection())
				->setShvPath(sites_log_file)
				->setMethod("read")
				->setParams(cp::RpcValue::Map{{"offset", cp::RpcValue::Int(local_size)}});
			call->start();
			auto [result, retrieve_error] = co_await qCoro(call, &shv::iotqt::rpc::RpcCall::maybeResult);
			if (!retrieve_error.isEmpty()) {
				journalError() << "Couldn't retrieve" << sites_log_file << ":" << retrieve_error;
				co_return;
			}

			journalDebug() << "Got" << sites_log_file;
			m_downloadedFiles.insert(full_file_name, result);
		}

		writeFiles();

		// We'll only trim if we actually downloaded some files, otherwise the trim algorithm will screw us over,
		// because of the "last millisecond algorithm".
		if (!file_list.asList().empty()) {
			m_node->trimDirtyLog(slave_hp_path);
		}
	}

	QCoro::Task<void, QCoro::TaskOptions<QCoro::Options::AbortOnException>> syncFiles()
	{
		using shv::coreqt::Utils;
		for (const auto& slave_hp : m_slaveHps) {
			auto slave_hp_path_qstr = QString::fromStdString(slave_hp.shv_path);

			if (!m_shvPath.isEmpty() && !m_shvPath.startsWith(slave_hp.shv_path.c_str())) {
				continue;
			}

			if (m_syncInProgress.value(slave_hp_path_qstr, false)) {
				journalInfo() << slave_hp_path_qstr << "is already being synced, skipping";
				continue;
			}

			m_syncInProgress[slave_hp_path_qstr] = true;
			QScopeGuard lock_guard([this, &slave_hp_path_qstr] {
				m_syncInProgress[slave_hp_path_qstr] = false;
			});

			// Now that we know that shvPath to-be-synced starts with the slave hp path, we need to check whether we're
			// connecting through a slave HP or connecting directly to a device.
			//
			// A special case is when m_shvPath is empty.
			SyncType sync_type =
				slave_hp.is_leaf && (slave_hp_path_qstr.size() == m_shvPath.size() || m_shvPath.isEmpty()) ? SyncType::Device : SyncType::Slave;

			// We shouldn't sync pushlogs, if they're our directly connected device.
			if (sync_type == SyncType::Device && slave_hp.log_type == LogType::PushLog) {
				continue;
			}

			if (sync_type == SyncType::Device && slave_hp.log_type == LogType::Legacy) {
				co_await doLegacySync(slave_hp_path_qstr, sync_type);
			} else {
				co_await doSync(slave_hp_path_qstr, sync_type);
			}
		}

		m_callback({});
		deleteLater();
	}

private:
	ShvJournalNode* m_node;
	std::vector<SlaveHpInfo> m_slaveHps;
	QString m_shvPath;
	int current_memory_usage = 0;


	QMap<QString, cp::RpcValue> m_downloadedFiles;

	QString m_cacheDirPath;

	std::function<void(cp::RpcResponse::Error)> m_callback;
	QMap<QString, bool>& m_syncInProgress;
};

void ShvJournalNode::syncLog(const std::string& shv_path, const std::function<void(cp::RpcResponse::Error)> cb)
{
	new FileSyncer(this, m_slaveHps, shv_path, m_cacheDirPath, cb, m_syncInProgress);
}

cp::RpcValue ShvJournalNode::callMethodRq(const cp::RpcRequest &rq)
{
	auto method = rq.method().asString();
	if (method == "syncLog") {
		journalInfo() << "Syncing shvjournal" << m_remoteLogShvPath;
		auto respond = [rq] (auto error) {
			auto response = rq.makeResponse();
			if (!error.empty()) {
				response.setError(error);
			} else {
				response.setResult("All files have been synced");
			}

			HistoryApp::instance()->rpcConnection()->sendMessage(response);
		};

		syncLog("", respond);

		return {};
	}

	return Super::callMethodRq(rq);
}

#include "shvjournalnode.moc"
