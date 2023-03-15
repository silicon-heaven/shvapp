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
#include <QTimer>

#define journalDebug() shvCDebug("historyjournal")
#define journalInfo() shvCInfo("historyjournal")

namespace cp = shv::chainpack;
namespace {
constexpr auto SYNCLOG_DESC = R"(syncLog - triggers a manual sync
With no param, all sites are synced. This can (and will) take some time.
With a string param, only the subtree signified by the string is synced.
syncLog also takes a map param in this format: {
	waitForFinished: bool // the method waits until the whole operation is finished and only then returns a response
	shvPath: string // the subtree to be synced
}

Returns: a list of all leaf sites that will be synced
)";

constexpr auto SYNCINFO_DESC = R"(syncInfo - returns info about sites' sync status
Optionally takes a string that filters the sites by prefix.

Returns: a map where they is the path of the site and the value is a map with a status string and a last updated timestamp.
)";
const std::vector<cp::MetaMethod> methods {
	{"syncLog", cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_WRITE, SYNCLOG_DESC},
	{"syncInfo", cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ, SYNCINFO_DESC},
};

const auto DIRTY_FILENAME = "dirtylog";

template <typename StringType>
std::string dirty_log_path(const StringType& cache_dir_path)
{
	return shv::coreqt::utils::joinPath<std::string>(cache_dir_path, DIRTY_FILENAME);
}

template <typename StringType>
std::string get_cache_dir_path(const QString& cache_root, const StringType& slave_hp_path)
{
	return shv::coreqt::utils::joinPath<std::string>(cache_root, slave_hp_path);
}

}

ShvJournalNode::ShvJournalNode(const std::vector<SlaveHpInfo>& slave_hps, const std::set<std::string>& leaf_nodes, ShvNode* parent)
	: Super(QString::fromStdString(HistoryApp::instance()->cliOptions()->journalCacheRoot()), "_shvjournal", parent)
	, m_slaveHps(slave_hps)
	, m_leafNodes(leaf_nodes)
	, m_cacheDirPath(QString::fromStdString(HistoryApp::instance()->cliOptions()->journalCacheRoot()))
{
	QDir(m_cacheDirPath).mkpath(".");
	auto conn = HistoryApp::instance()->rpcConnection();

	auto now = shv::chainpack::RpcValue::DateTime::now();
	for (const auto& slave_hp : m_slaveHps) {
		m_syncInfo.setValue(slave_hp.shv_path, shv::chainpack::RpcValue::Map {
			{"timestamp", now},
			{"status", shv::chainpack::RpcValue::List{"Unknown"}},
		});
	}

	connect(conn, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvJournalNode::onRpcMessageReceived);

	conn->callMethodSubscribe("shv", "mntchng");
	for (const auto& it : slave_hps) {

		if (it.log_type != LogType::PushLog) {
			conn->callMethodSubscribe(it.shv_path, "chng");
		}
	}

	auto tmr = new QTimer(this);
	connect(tmr, &QTimer::timeout, [this, sync_iterator = m_slaveHps.begin(), dirtylog_age_cache = std::map<std::string, int64_t>{}] () mutable {
		if (sync_iterator == m_slaveHps.end()) {
			sync_iterator = m_slaveHps.begin();
		}

		auto current_slave_hp_path = sync_iterator->shv_path;
		auto current_slave_cache_dir_path = sync_iterator->cache_dir_path;
		if (dirtylog_age_cache.find(current_slave_hp_path) == dirtylog_age_cache.end()) {
			dirtylog_age_cache.emplace(current_slave_hp_path, 0);

			auto journal_dir_path = get_cache_dir_path(m_cacheDirPath, current_slave_cache_dir_path);
			if (QFile(QString::fromStdString(dirty_log_path(journal_dir_path))).exists()) {
				if (auto reader = shv::core::utils::ShvJournalFileReader(dirty_log_path(journal_dir_path)); reader.next()) {
					dirtylog_age_cache[current_slave_hp_path] = reader.entry().epochMsec;
				}
			}
		}

		if ((QDateTime::currentDateTime().toMSecsSinceEpoch() - dirtylog_age_cache[current_slave_hp_path]) > HistoryApp::instance()->cliOptions()->logMaxAge() * 1000) {
			HistoryApp::instance()->shvJournalNode()->syncLog(sync_iterator->shv_path, [] (const auto&) {}, [] {});
			dirtylog_age_cache.erase(current_slave_hp_path);
		}

		++sync_iterator;
	});
	tmr->start(HistoryApp::instance()->cliOptions()->syncIteratorInterval() * 1000);
}

namespace {
}

void ShvJournalNode::onRpcMessageReceived(const cp::RpcMessage &msg)
{
	if (msg.isSignal()) {
		cp::RpcSignal ntf(msg);
		auto path = ntf.shvPath().asString();
		auto method = ntf.method().asString();
		auto params = ntf.params();
		auto value = ntf.value();

		if (method == "chng") {
			auto it = std::find_if(m_slaveHps.begin(), m_slaveHps.end(), [&path] (const auto& slave_hp) {
				return path.starts_with(slave_hp.shv_path);
			});

			if (it != m_slaveHps.end()) {
				{
					if (path.at(it->shv_path.size()) != '/') {
						shvWarning() << "Discarding notification with a top-level node path. Offending path was:" << it->shv_path;
						return;
					}

					auto longest_prefix = shv::core::utils::findLongestPrefix(m_leafNodes, path);
					// get rid of the shv prefix
					auto longest_prefix_without_shv = longest_prefix->substr(4);

					auto writer = shv::core::utils::ShvJournalFileWriter(dirty_log_path(get_cache_dir_path(m_cacheDirPath, longest_prefix_without_shv)));
					auto path_without_prefix = path.substr(longest_prefix->size() + 1 /* for the slash */);
					auto data_change = shv::chainpack::DataChange::fromRpcValue(ntf.params());

					auto entry = shv::core::utils::ShvJournalEntry(path_without_prefix, data_change.value()
																   , shv::core::utils::ShvJournalEntry::DOMAIN_VAL_CHANGE
																   , shv::core::utils::ShvJournalEntry::NO_SHORT_TIME
																   , shv::core::utils::ShvJournalEntry::NO_VALUE_FLAGS
																   , data_change.epochMSec());
					writer.append(entry);

				}
			}
		}

		if (method == "mntchng") {
			for (const auto& slave_hp : m_slaveHps) {
				if (slave_hp.shv_path.starts_with(path) &&
					slave_hp.log_type != LogType::PushLog &&
					!m_syncInProgress.value(QString::fromStdString(slave_hp.shv_path), false) &&
					params.toBool() == true) {
					journalInfo() << "mntchng on" << slave_hp.shv_path << "syncing its journal";
					syncLog(slave_hp.shv_path, [] (const auto&) {}, [signal_path = slave_hp.shv_path] () {
						HistoryApp::instance()->rpcConnection()->sendShvSignal(signal_path, "logReset");
					});
				}
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
enum class Overwrite {
	Yes,
	No
};

void do_write_entries_to_file(const QString& file_path, const std::vector<shv::core::utils::ShvJournalEntry>& entries, const Overwrite overwrite)
{
	journalDebug() << "Writing" << entries.size() << "entries to" << file_path;
	if (overwrite == Overwrite::Yes) {
		QFile(file_path).remove();
	}

	shv::core::utils::ShvJournalFileWriter writer(file_path.toStdString());
	for (const auto& entry : entries) {
		writer.append(entry);
	}
}
}

void ShvJournalNode::trimDirtyLog(const QString& slave_hp_path, const QString& cache_dir_path)
{
	journalInfo() << "Trimming dirty log for" << slave_hp_path;
	using shv::coreqt::Utils;
	auto journal_dir_path = cache_dir_path;
	QDir cache_dir(journal_dir_path);
	auto entries = cache_dir.entryList(QDir::NoDotAndDotDot | QDir::Files, QDir::Name | QDir::Reversed);

	if (entries.empty() || entries.at(0) != DIRTY_FILENAME) {
		journalDebug() << "No dirty log, nothing to trim";
		return;
	}

	if (entries.size() == 1) {
		journalDebug() << "No logs to use for trimming";
		return;
	}

	if (!shv::core::utils::ShvJournalFileReader(dirty_log_path(journal_dir_path)).next()) {
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

	do_write_entries_to_file(cache_dir.filePath(entries.at(1)), newest_file_entries, Overwrite::Yes);

	// Now filter dirty log's newer events.
	shv::core::utils::ShvJournalFileReader reader(dirty_log_path(journal_dir_path));
	std::vector<shv::core::utils::ShvJournalEntry> new_dirty_log_entries;
	while (reader.next()) {
		if (reader.entry().epochMsec >= newest_entry_msec) {
			new_dirty_log_entries.push_back(reader.entry());
		}
	}

	do_write_entries_to_file(QString::fromStdString(dirty_log_path(journal_dir_path)), new_dirty_log_entries, Overwrite::Yes);
}

namespace {
const auto RECORD_COUNT_LIMIT = 5000;
const auto MAX_ENTRIES_PER_FILE = 50000;
const auto SYNCER_MEMORY_LIMIT = 1024 /*B*/ * 1024 /*kB*/ * 100 /*MB*/;

const auto LS_FILES_RESPONSE_FILENAME = 0;
const auto LS_FILES_RESPONSE_FILETYPE = 1;
const auto LS_FILES_RESPONSE_FILESIZE = 2;

void writeEntriesToFile(ShvJournalNode* node, const std::vector<shv::core::utils::ShvJournalEntry>& downloaded_entries, const QString& slave_hp_path, const QString& cache_dir_path, const std::optional<QString>& file_name_hint)
{
	journalInfo() << "Writing" << downloaded_entries.size() << "entries for" << cache_dir_path;
	if (downloaded_entries.empty()) {
		return;
	}

	QDir dir(cache_dir_path);

	auto file_name = dir.filePath([&] {
		if (file_name_hint) {
			return file_name_hint.value();
		}

		return QString::fromStdString(shv::core::utils::ShvJournalFileReader::msecToBaseFileName(downloaded_entries.front().epochMsec)) + ".log2";
	}());

	do_write_entries_to_file(file_name, downloaded_entries, Overwrite::No);
	node->trimDirtyLog(slave_hp_path, cache_dir_path);
}
}

class LegacyFileSyncerImpl : public QObject {
	Q_OBJECT
public:
	LegacyFileSyncerImpl(ShvJournalNode* node, const QString& slave_hp_path, const QString& cache_dir_path)
		: slave_hp_path(slave_hp_path)
		, cache_dir_path(cache_dir_path)
		, node(node)
	{
		journalInfo() << "Syncing" << slave_hp_path << "via legacy getLog";
		using shv::coreqt::Utils;
		get_log_params.withSnapshot = true;
		get_log_params.recordCountLimit = RECORD_COUNT_LIMIT;
		get_log_params.since = shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(QDateTime::currentDateTime().addSecs(- HistoryApp::instance()->cliOptions()->cacheInitMaxAge()).toMSecsSinceEpoch());

		QDir cache_dir(cache_dir_path);
		auto entry_list = cache_dir.entryList(QDir::NoDotAndDotDot | QDir::Files, QDir::Name | QDir::Reversed);
		if (!entry_list.empty()) {
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

					// No fancy algorithm for appending the files: we'll only append if the existing file can contain
					// the whole RECORD_COUNT_LIMIT records.
					if (newest_file_entries.size() + RECORD_COUNT_LIMIT < MAX_ENTRIES_PER_FILE) {
						newest_file_entry_count = static_cast<int>(newest_file_entries.size());
						file_name_hint = newest_file_name;
						get_log_params.withSnapshot = false;
					}
				}
			}
		}
		promise.start();
		syncNext();
	}

	QFuture<void> getFuture()
	{
		return promise.future();
	}

	~LegacyFileSyncerImpl() override
	{
		writeEntriesToFile(node, downloaded_entries, slave_hp_path, cache_dir_path, file_name_hint);
		promise.finish();
	}
private:
	void syncNext()
	{
		auto call = shv::iotqt::rpc::RpcCall::create(HistoryApp::instance()->rpcConnection())
			->setShvPath(slave_hp_path)
			->setMethod("getLog")
			->setParams(get_log_params.toRpcValue())
			->setTimeout(10000);
		call->start();
		connect(call, &shv::iotqt::rpc::RpcCall::maybeResult, [this] (const auto& result, const auto& error) {
			if (!error.isEmpty()) {
				auto err = "Error retrieving logs via getLog for: " + slave_hp_path + " " + error;
				shvError() << err;
				node->appendSyncStatus(slave_hp_path, err.toStdString());
				deleteLater();
				return;
			}

			get_log_params.withSnapshot = false;

			shv::core::utils::ShvMemoryJournal result_log;
			result_log.loadLog(result);
			if (result_log.isEmpty()) {
				deleteLater();
				return;
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
				deleteLater();
				return;
			}

			if (downloaded_entries.size() + newest_file_entry_count > MAX_ENTRIES_PER_FILE) {
				writeEntriesToFile(node, downloaded_entries, slave_hp_path, cache_dir_path, file_name_hint);
				downloaded_entries.clear();
				newest_file_entry_count = 0;
				file_name_hint.reset();
				get_log_params.withSnapshot = true;
			}
			get_log_params.since = remote_entries.back().dateTime();
			syncNext();
		});
	}

	QPromise<void> promise;
	QString slave_hp_path;
	QString cache_dir_path;
	ShvJournalNode* node;
	std::vector<shv::core::utils::ShvJournalEntry> downloaded_entries;
	int newest_file_entry_count = 0;
	std::optional<QString> file_name_hint;
	shv::core::utils::ShvGetLogParams get_log_params;
};

class FileSyncer : public QObject {
	Q_OBJECT
public:
	FileSyncer(
		ShvJournalNode* node,
		const std::string& shv_path,
		const std::function<void(const shv::chainpack::RpcValue::List&)>& site_list_cb,
		const std::function<void()>& success_cb)
		: m_node(node)
		, m_shvPath(QString::fromStdString(shv_path))
		, m_siteListCb(site_list_cb)
		, m_successCb(success_cb)
	{
		syncFiles();
	}

	void writeFiles()
	{
		QMapIterator iter(m_downloadedFiles);
		while (iter.hasNext()) {
			iter.next();
			QFile log_file(iter.key());
			QFileInfo(log_file).dir().mkpath(".");
			if (log_file.open(QFile::WriteOnly | QFile::Append)) {
				auto blob = iter.value().asBlob();
				journalDebug() << "Writing" << iter.key();
				log_file.write(reinterpret_cast<const char*>(blob.data()), blob.size());
			} else {
				auto err = "Couldn't open " + iter.key() + " for writing";
				shvError() << err;
				m_node->appendSyncStatus(m_shvPath, err.toStdString());
			}

		}

		m_downloadedFiles.clear();
		current_memory_usage = 0;
	}

	enum class SyncType {
		Device,
		HP3
	};

	QFuture<void> impl_doSync(const QString& slave_hp_path, const QString& cache_dir_path, const QString& shvjournal_shvpath, const QString& path_prefix)
	{
		auto call = shv::iotqt::rpc::RpcCall::create(HistoryApp::instance()->rpcConnection())
			->setShvPath(shvjournal_shvpath)
			->setMethod("lsfiles")
			->setParams(cp::RpcValue::Map{{"size", true}});
		call->start();

		QPromise<void> promise;
		auto future = promise.future();
		promise.start();

		connect(call, &shv::iotqt::rpc::RpcCall::maybeResult, this, [this, shvjournal_shvpath, cache_dir_path, slave_hp_path, path_prefix, promise = std::move(promise)] (const auto& file_list, const auto& error) mutable {
			if (!error.isEmpty()) {
				auto err = "Couldn't retrieve filelist from: " + shvjournal_shvpath + " " + error;
				shvError() << err;
				m_node->appendSyncStatus(slave_hp_path, err.toStdString());
				promise.finish();
				return;
			}

			journalDebug() << "Got filelist from" << slave_hp_path;

			QVector<QFuture<void>> all_files_synced;
			if (!file_list.asList().empty() && file_list.asList().front().asList().at(LS_FILES_RESPONSE_FILETYPE) == "d") {
				for (const auto& current_directory : file_list.asList()) {
					auto dir_name = QString::fromStdString(current_directory.asList().at(LS_FILES_RESPONSE_FILENAME).asString());
					all_files_synced.push_back(impl_doSync(slave_hp_path, cache_dir_path, shv::coreqt::utils::joinPath(shvjournal_shvpath, dir_name), shv::coreqt::utils::joinPath(path_prefix, dir_name)));
				}
				QtFuture::whenAll(all_files_synced.begin(), all_files_synced.end()).then([promise = std::move(promise)] (const auto&) mutable {
					promise.finish();
				});
				return;
			}

			QDir cache_dir(cache_dir_path);
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
				if (file_name == DIRTY_FILENAME) {
					continue;
				}

				if (shv::core::utils::ShvJournalFileReader::fileNameToFileMsec(file_name.toStdString()) < newest_file_name_ms) {
					journalDebug() << "Skipping" << file_name << "because it's older than our newest file" << newest_file_name;
					continue;
				}

				auto full_file_name = QDir(cache_dir_path).filePath(shv::coreqt::utils::joinPath(path_prefix, file_name));
				QFile file(full_file_name);
				auto local_size = file.size();
				auto remote_size = current_file.asList().at(LS_FILES_RESPONSE_FILESIZE).toInt();

				journalInfo() << "Syncing file" << full_file_name << "remote size:" << remote_size << "local size:" << (file.exists() ? QString::number(local_size) : "<doesn't exist>");
				if (file.exists()) {
					if (local_size == remote_size) {
						continue;
					}
				}

				auto sites_log_file = shv::coreqt::utils::joinPath(shvjournal_shvpath, file_name);
				journalDebug() << "Retrieving" << sites_log_file << "offset:" << local_size;
				auto call = shv::iotqt::rpc::RpcCall::create(HistoryApp::instance()->rpcConnection())
					->setShvPath(sites_log_file)
					->setMethod("read")
					->setParams(cp::RpcValue::Map{{"offset", cp::RpcValue::Int(local_size)}});
				call->start();
				QPromise<void> file_synced_promise;
				auto future = file_synced_promise.future();
				file_synced_promise.start();
				connect(call, &shv::iotqt::rpc::RpcCall::maybeResult, this, [this, slave_hp_path, sites_log_file, full_file_name, file_synced_promise = std::move(file_synced_promise)] (const auto& result, const auto& retrieve_error) mutable {
					if (!retrieve_error.isEmpty()) {
						auto err = "Couldn't retrieve " + sites_log_file + ": " + retrieve_error;
						shvError() << err;
						m_node->appendSyncStatus(slave_hp_path, err.toStdString());
						file_synced_promise.finish();
						return;
					}

					journalDebug() << "Got" << sites_log_file;
					m_downloadedFiles.insert(full_file_name, result);
					file_synced_promise.finish();
				});
				all_files_synced.push_back(future);
			}

			QtFuture::whenAll(all_files_synced.begin(), all_files_synced.end()).then([this, slave_hp_path, cache_dir_path, file_list, promise = std::move(promise)] (const auto&) mutable {
				writeFiles();

				// We'll only trim if we actually downloaded some files, otherwise the trim algorithm will screw us over,
				// because of the "last millisecond algorithm".
				if (!file_list.asList().empty()) {
					m_node->trimDirtyLog(slave_hp_path, cache_dir_path);
				}

				m_node->appendSyncStatus(slave_hp_path, "Syncing successful");
				promise.finish();
			});
		});
		return future;
	}

	QFuture<void> doSync(const QString& slave_hp_path, const QString& cache_dir_path, const SyncType sync_type)
	{
		journalInfo() << "Syncing" << slave_hp_path << "via file synchronization";
		auto shvjournal_suffix =
			sync_type == SyncType::Device ?
			".app/shvjournal" :
			shv::coreqt::utils::joinPath(".local/history/_shvjournal", m_shvPath.mid(slave_hp_path.size()));
		auto shvjournal_shvpath = shv::coreqt::utils::joinPath(slave_hp_path, shvjournal_suffix);

		return impl_doSync(slave_hp_path, cache_dir_path, shvjournal_shvpath, "");
	}

	void syncFiles()
	{
		using shv::coreqt::Utils;
		QVector<QFuture<void>> all_synced;
		shv::chainpack::RpcValue::List sites_to_be_synced;
		for (const auto& slave_hp : m_node->slaveHps()) {
			auto slave_hp_path_qstr = QString::fromStdString(slave_hp.shv_path);

			if (!m_shvPath.isEmpty() && !slave_hp.shv_path.starts_with(m_shvPath.toStdString())) {
				continue;
			}

			journalDebug() << "Found matching site to sync:" << slave_hp.shv_path;

			if (m_node->syncInProgress().value(slave_hp_path_qstr, false)) {
				journalInfo() << slave_hp_path_qstr << "is already being synced, skipping";
				continue;
			}

			m_node->syncInProgress()[slave_hp_path_qstr] = true;
			QScopeGuard lock_guard([this, &slave_hp_path_qstr] {
				m_node->syncInProgress()[slave_hp_path_qstr] = false;
			});

			// We know all the leaf nodes, so let's check what's our sync type.
			auto sync_type = m_node->leafNodes().contains(slave_hp.shv_path) ? SyncType::Device : SyncType::HP3;

			// We shouldn't sync pushlogs, if they're our directly connected device.
			if (sync_type == SyncType::Device && slave_hp.log_type == LogType::PushLog) {
				continue;
			}
			sites_to_be_synced.push_back(slave_hp.shv_path);

			m_node->resetSyncStatus(slave_hp_path_qstr);
			m_node->appendSyncStatus(slave_hp_path_qstr, "Syncing");
			if (sync_type == SyncType::Device && slave_hp.log_type == LogType::Legacy) {
				all_synced.push_back((new LegacyFileSyncerImpl(m_node, slave_hp_path_qstr, slave_hp.cache_dir_path))->getFuture());
			} else {
				all_synced.push_back(doSync(slave_hp_path_qstr, slave_hp.cache_dir_path, sync_type));
			}
		}

		if (!all_synced.empty()) {
			m_siteListCb(sites_to_be_synced);
			QtFuture::whenAll(all_synced.begin(), all_synced.end()).then([this] (const auto&) {
				m_successCb();
				deleteLater();
			});
		}
	}

private:
	ShvJournalNode* m_node;
	const QString m_shvPath;
	int current_memory_usage = 0;

	QMap<QString, cp::RpcValue> m_downloadedFiles;

	const std::function<void(const shv::chainpack::RpcValue::List&)> m_siteListCb;
	const std::function<void()> m_successCb;
};

void ShvJournalNode::syncLog(const std::string& shv_path, const std::function<void(const shv::chainpack::RpcValue::List&)> site_list_cb, const std::function<void()> success_cb)
{
	new FileSyncer(this, shv_path, site_list_cb, success_cb);
}

cp::RpcValue ShvJournalNode::callMethodRq(const cp::RpcRequest &rq)
{
	auto method = rq.method().asString();
	if (method == "syncLog") {
		auto params = rq.params();
		journalInfo() << "Syncing shvjournal" << m_remoteLogShvPath;
		auto sites_resp = std::make_shared<shv::chainpack::RpcValue>();

		auto onSites = [sites_resp, params, rq, shv_path = rq.params().asString()] (const shv::chainpack::RpcValue::List& sites) {
			if (!params.asMap().value("waitForFinished", false).toBool()) {
				auto response = rq.makeResponse();
				response.setResult(sites);
				HistoryApp::instance()->rpcConnection()->sendMessage(response);
			} else {
				*sites_resp = sites;
			}
		};
		auto onSuccess = [sites_resp, params, rq] {
			if (params.asMap().value("waitForFinished", false).toBool()) {
				auto response = rq.makeResponse();
				response.setResult(*sites_resp);
				HistoryApp::instance()->rpcConnection()->sendMessage(response);
			}
		};

		syncLog(params.isString() ? params.asString() : params.asMap().value("shvPath", "").asString(), onSites, onSuccess);

		return {};
	}

	if (method == "syncInfo") {
		auto sync_info = syncInfo();
		if (auto filter = rq.params().asString(); !filter.empty()) {
			for (auto it = sync_info.begin(); it != sync_info.end(); /*nothing*/) {
				if (!it->first.starts_with(filter)) {
					it = sync_info.erase(it);
					continue;
				}
				it++;
			}
		}
		return sync_info;
	}

	return Super::callMethodRq(rq);
}

const QString& ShvJournalNode::cacheDirPath() const
{
	return m_cacheDirPath;
}

QMap<QString, bool>& ShvJournalNode::syncInProgress()
{
	return m_syncInProgress;
}

void ShvJournalNode::resetSyncStatus(const QString& shv_path)
{
	m_syncInfo.setValue(shv_path.toStdString(), shv::chainpack::RpcValue::Map {
		{"status", shv::chainpack::RpcValue::List{}},
		{"timestamp", shv::chainpack::RpcValue::DateTime::now()}
	});
}

void ShvJournalNode::appendSyncStatus(const QString& shv_path, const std::string& status)
{
	m_syncInfo.at(shv_path.toStdString()).set("timestamp", shv::chainpack::RpcValue::DateTime::now());
	auto current_status = m_syncInfo.at(shv_path.toStdString()).asMap().value("status").asList();
	current_status.push_back(status);
	m_syncInfo.at(shv_path.toStdString()).set("status", current_status);
}

const shv::chainpack::RpcValue::Map& ShvJournalNode::syncInfo()
{
	return m_syncInfo;
}

const std::vector<SlaveHpInfo>& ShvJournalNode::slaveHps() const
{
	return m_slaveHps;
}

const std::set<std::string>& ShvJournalNode::leafNodes() const
{
	return m_leafNodes;
}

#include "shvjournalnode.moc"
