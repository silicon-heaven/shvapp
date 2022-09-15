#include "shvjournalnode.h"
#include "historyapp.h"
#include "appclioptions.h"

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

#include <QCoroSignal>

#define journalDebug() shvCDebug("historyjournal")
#define journalInfo() shvCInfo("historyjournal")
#define journalError() shvCError("historyjournal")

namespace cp = shv::chainpack;
namespace {
static std::vector<cp::MetaMethod> methods {
	{"logSize", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{"sanitizeLog", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_WRITE},
	{"getLog", cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{"syncLog", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_WRITE},
};
}

ShvJournalNode::ShvJournalNode(const std::vector<SlaveHpInfo>& slave_hps, ShvNode* parent)
	: Super(QString::fromStdString(HistoryApp::instance()->cliOptions()->journalCacheRoot()), "shvjournal", parent)
	, m_slaveHps(slave_hps)
	, m_cacheDirPath(QString::fromStdString(HistoryApp::instance()->cliOptions()->journalCacheRoot()))
	, m_dirtyLogFirstTimestamp(-1)
{
	QDir(m_cacheDirPath).mkpath(".");
	auto conn = HistoryApp::instance()->rpcConnection();

	connect(conn, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvJournalNode::onRpcMessageReceived);

	for (const auto& it : slave_hps) {
		conn->callMethodSubscribe(it.shv_path, "mntchng");

		if (it.log_type != LogType::PushLog) {
			conn->callMethodSubscribe(it.shv_path, "chng");
		}
	}
}

namespace {
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
		auto value = ntf.value();

		auto it = std::find_if(m_slaveHps.begin(), m_slaveHps.end(), [&path] (const auto& slave_hp) {
			return path.starts_with(slave_hp.shv_path);
		});

		if (it != m_slaveHps.end()) {
			if (method == "chng") {
				{
					auto writer = shv::core::utils::ShvJournalFileWriter(dirty_log_path(shv::coreqt::Utils::joinPath(m_cacheDirPath, QString::fromStdString(it->shv_path))));
					auto path_without_prefix = path.substr(it->shv_path.size());
					auto data_change = shv::chainpack::DataChange::fromRpcValue(ntf.params());

					auto entry = shv::core::utils::ShvJournalEntry(path_without_prefix, data_change.value()
															, shv::core::utils::ShvJournalEntry::DOMAIN_VAL_CHANGE
															, shv::core::utils::ShvJournalEntry::NO_SHORT_TIME
															, shv::core::utils::ShvJournalEntry::NO_VALUE_FLAGS
															, data_change.epochMSec());
					writer.append(entry);

				}

				if (m_dirtyLogFirstTimestamp == -1) {
					shv::core::utils::ShvJournalFileReader reader(dirty_log_path(shv::coreqt::Utils::joinPath(m_cacheDirPath, QString::fromStdString(it->shv_path))));
					reader.next(); // There must be at least one entry, because I've just written it
					m_dirtyLogFirstTimestamp = reader.entry().epochMsec;
				}

				if (QDateTime::currentMSecsSinceEpoch() - HistoryApp::instance()->cliOptions()->logMaxAge() * 1000 > m_dirtyLogFirstTimestamp) {
					journalInfo() << "Log is too old, syncing journal" << it->shv_path;
					syncLog(it->shv_path, [] (auto /*error*/) {
					});
				}
			}

			if (method == "mntchng") {
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
auto read_entries_from_file(const QString& file_path)
{
	std::vector<shv::core::utils::ShvJournalEntry> res;
	shv::core::utils::ShvJournalFileReader reader(file_path.toStdString());
	while (reader.next()) {
		res.push_back(reader.entry());
	}

	return res;
}

void write_entries_to_file(const QString& file_path, const std::vector<shv::core::utils::ShvJournalEntry>& entries)
{
	QFile(file_path).remove();
	shv::core::utils::ShvJournalFileWriter writer(file_path.toStdString());
	for (const auto& entry : entries) {
		writer.append(entry);
	}
}
}

void ShvJournalNode::trimDirtyLog(const QString& cache_dir_path)
{
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

	auto newest_file_entries = read_entries_from_file(cache_dir.filePath(entries.at(1)));
	auto newest_entry_msec = newest_file_entries.back().epochMsec;
	// It is possible that the newest logfile contains multiple entries with the same timestamp. Because it could
	// have been written (by shvagent) in between two events that happenede in the same millisecond, we can't be
	// sure whether we have all events from the last millisecond. Because of that we will discard the last
	// millisecond from the synced log, and keep it in the dirty log.
	newest_file_entries.erase(std::find_if(newest_file_entries.begin(), newest_file_entries.end(), [newest_entry_msec] (const auto& entry) {
		return entry.epochMsec == newest_entry_msec;
	}), newest_file_entries.end());
	write_entries_to_file(cache_dir.filePath(entries.at(1)), newest_file_entries);

	// Now filter dirty log's newer events.
	shv::core::utils::ShvJournalFileReader reader(dirty_log_path(cache_dir_path));
	std::vector<shv::core::utils::ShvJournalEntry> new_dirty_log_entries;
	bool first = true;
	while (reader.next()) {
		if (first) {
			m_dirtyLogFirstTimestamp = reader.entry().epochMsec;
			journalDebug() << "Dirty logfile first entry timestamp" << m_dirtyLogFirstTimestamp;
			first = false;
		}

		if (reader.entry().epochMsec >= newest_entry_msec) {
			new_dirty_log_entries.push_back(reader.entry());
		}
	}

	write_entries_to_file(QString::fromStdString(dirty_log_path(cache_dir_path)), new_dirty_log_entries);
}

class FileSyncer : public QObject {
	Q_OBJECT
public:
	FileSyncer(ShvJournalNode* node, std::vector<SlaveHpInfo> slave_hps, const std::string& shv_path, const QString& cache_dir_path, const std::function<void(cp::RpcResponse::Error)>& callback)
		: m_node(node)
		, m_slaveHps(slave_hps)
		, m_shvPath(QString::fromStdString(shv_path))
		, m_cacheDirPath(cache_dir_path)
		, m_callback(callback)
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
	}

	QCoro::Task<void, QCoro::TaskOptions<QCoro::Options::AbortOnException>> syncFiles()
	{
		using shv::coreqt::Utils;
		for (const auto& slave_hp : m_slaveHps) {
			auto slave_hp_path_qstr = QString::fromStdString(slave_hp.shv_path);
			if (!m_shvPath.isEmpty() && !m_shvPath.startsWith(slave_hp.shv_path.c_str())) {
				continue;
			}

			// Now that we know that shvPath to-be-synced starts with the slave hp path, we need to check whether we're
			// connecting through a slave HP or connecting directly to a device.
			//
			// A special case is when m_shvPath is empty.
			enum class SyncType {Device, Slave} sync_type =
				slave_hp.is_leaf && (slave_hp_path_qstr.size() == m_shvPath.size() || m_shvPath.isEmpty()) ? SyncType::Device : SyncType::Slave;

			// We shouldn't sync pushlogs, if they're our directly connected device.
			if (sync_type == SyncType::Device && slave_hp.log_type == LogType::PushLog) {
				continue;
			}

			auto shvjournal_suffix =
				sync_type == SyncType::Device ?
				".app/shvjournal" :
				Utils::joinPath(".local/history/shvjournal", m_shvPath.remove(0, slave_hp_path_qstr.size()));
			auto shvjournal_shvpath = Utils::joinPath(slave_hp_path_qstr, shvjournal_suffix);

			auto call = shv::iotqt::rpc::RpcCall::create(HistoryApp::instance()->rpcConnection())
				->setShvPath(shvjournal_shvpath)
				->setMethod("lsfiles")
				->setParams(cp::RpcValue::Map{{"size", true}});
			call->start();

			auto [file_list, error] = co_await qCoro(call, &shv::iotqt::rpc::RpcCall::maybeResult);
			if (!error.isEmpty()) {
				journalError() << error;
				m_callback(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, "Couldn't retrieve filelist from the device"));
				continue;
			}

			journalDebug() << "Got filelist from" << slave_hp.shv_path;

			for (const auto& current_file : file_list.asList()) {
				auto file_name = QString::fromStdString(current_file.asList().at(0).asString());
				if (file_name == "dirty.log2") {
					continue;
				}
				auto full_file_name = QDir(Utils::joinPath(m_cacheDirPath, slave_hp_path_qstr)).filePath(file_name);
				QFile file(full_file_name);
				auto local_size = file.size();
				auto remote_size = current_file.asList().at(2).toInt();

				journalInfo() << "Syncing file" << full_file_name << "remote size:" << remote_size << "local size:" << (file.exists() ? QString::number(local_size) : "<doesn't exist>");
				if (file.exists()) {
					if (local_size == remote_size) {
						continue;
					}
				}

				auto sites_log_file = Utils::joinPath(shvjournal_shvpath, file_name);
				journalDebug() << "Retrieving" << sites_log_file << "offset:" << local_size;
				auto call = shv::iotqt::rpc::RpcCall::create(HistoryApp::instance()->rpcConnection())
					->setShvPath(sites_log_file)
					->setMethod("read")
					->setParams(cp::RpcValue::Map{{"offset", cp::RpcValue::Int(local_size)}});
				call->start();
				auto [result, error] = co_await qCoro(call, &shv::iotqt::rpc::RpcCall::maybeResult);
				if (!error.isEmpty()) {
					journalError() << "Couldn't retrieve" << sites_log_file << ":" << error;
					co_return;
				}

				journalDebug() << "Got" << sites_log_file;
				m_downloadedFiles.insert(full_file_name, result);
				m_node->trimDirtyLog(Utils::joinPath(m_cacheDirPath, slave_hp_path_qstr));
			}
		}

		writeFiles();
		m_callback({});
		deleteLater();
	}

private:
	ShvJournalNode* m_node;
	std::vector<SlaveHpInfo> m_slaveHps;
	QString m_shvPath;

	QMap<QString, cp::RpcValue> m_downloadedFiles;

	QString m_cacheDirPath;

	std::function<void(cp::RpcResponse::Error)> m_callback;
};

qint64 ShvJournalNode::calculateCacheDirSize() const
{
	journalDebug() << "Calculating cache directory size";
	QDirIterator iter(m_cacheDirPath, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);
	qint64 total_size = 0;
	while (iter.hasNext()) {
		QFile file(iter.next());
		total_size += file.size();
	}
	journalDebug() << "Cache directory size" << total_size;

	return total_size;
}

void ShvJournalNode::sanitizeSize()
{
	auto cache_dir_size = calculateCacheDirSize();
	auto cache_size_limit = HistoryApp::instance()->singleCacheSizeLimit();

	journalInfo() << "Sanitizing cache, path:" << m_cacheDirPath << "size" << cache_dir_size << "cacheSizeLimit" << cache_size_limit;

	QDir cache_dir(m_cacheDirPath);
	auto entries = cache_dir.entryList(QDir::NoDotAndDotDot | QDir::Files, QDir::Name);
	QStringListIterator iter(entries);

	while (cache_dir_size > cache_size_limit && iter.hasNext()) {
		QFile file(cache_dir.filePath(iter.next()));
		journalDebug() << "Removing" << file.fileName();
		cache_dir_size -= file.size();
		file.remove();
	}

	journalInfo() << "Sanitization done, path:" << m_cacheDirPath << "new size" << cache_dir_size << "cacheSizeLimit" << cache_size_limit;
}

namespace {
const auto RECORD_COUNT_LIMIT = 1000;
}

class LegacyFileSyncer : public QObject {
	Q_OBJECT
public:
	LegacyFileSyncer(ShvJournalNode* node, const QString& remote_log_shv_path, const QString& cache_dir_path, const std::function<void(cp::RpcResponse::Error)>& callback)
		: m_node(node)
		, m_untilParam(shv::chainpack::RpcValue::DateTime::now())
		, m_remoteLogShvPath(remote_log_shv_path)
		, m_cacheDirPath(cache_dir_path)
		, m_callback(callback)
	{
		QDir cache_dir(m_cacheDirPath);
		if (!cache_dir.isEmpty()) {
			auto entry_list = cache_dir.entryList(QDir::NoDotAndDotDot | QDir::Files, QDir::Name | QDir::Reversed);
			auto newest_file_name = entry_list.at(0) == DIRTY_FILENAME ? entry_list.at(1) : entry_list.at(0);
			auto newest_file_entries = read_entries_from_file(cache_dir.filePath(newest_file_name));

			m_sinceParam = shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(newest_file_entries.back().dateTime().msecsSinceEpoch() + 1);
		} else {
			m_sinceParam = shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(QDateTime::currentDateTime().addSecs(- HistoryApp::instance()->cliOptions()->legacyGetLogSinceInit()).toMSecsSinceEpoch());
		}

		downloadNextChunk();
	}

	Q_SIGNAL void chunkDone();

	void writeEntriesToFile()
	{
		auto file_name = QDir(m_cacheDirPath).filePath(QString::fromStdString(shv::core::utils::ShvJournalFileReader::msecToBaseFileName(m_downloadedEntries.front().epochMsec)) + ".log2");

		write_entries_to_file(file_name, m_downloadedEntries);
		m_node->trimDirtyLog(m_cacheDirPath);
		m_callback({});
		deleteLater();
	}

	QCoro::Task<void, QCoro::TaskOptions<QCoro::Options::AbortOnException>> downloadNextChunk()
	{
		shv::core::utils::ShvGetLogParams get_log_params;
		get_log_params.recordCountLimit = RECORD_COUNT_LIMIT;
		get_log_params.since = m_sinceParam;
		get_log_params.until = m_untilParam;
		while (true) {
			auto call = shv::iotqt::rpc::RpcCall::create(HistoryApp::instance()->rpcConnection())
				->setShvPath(m_remoteLogShvPath)
				->setMethod("getLog")
				->setParams(get_log_params.toRpcValue());
			call->start();
			auto [result, error] = co_await qCoro(call, &shv::iotqt::rpc::RpcCall::maybeResult);

			if (!error.isEmpty()) {
				journalError() << error;
				m_callback(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, "Couldn't retrieve logs from the device"));
				deleteLater();
			}

			shv::core::utils::ShvMemoryJournal result_log;
			result_log.loadLog(result);
			result_log.clearSnapshot();
			if (result_log.isEmpty()) {
				writeEntriesToFile();
				co_return;
			}

			const auto& remote_entries = result_log.entries();

			auto newest_entry = remote_entries.back().epochMsec;
			for (const auto& entry : remote_entries) {
				// We're skipping all entries from the last millisecond, and retrieve it again, otherwise we can't be
				// sure if we got all from the same millisecond.
				if (entry.epochMsec == newest_entry) {
					break;
				}
				m_downloadedEntries.push_back(entry);
			}

			if (remote_entries.size() < RECORD_COUNT_LIMIT) {
				writeEntriesToFile();
				co_return;
			}

			get_log_params.since = remote_entries.back().dateTime();
		}
	}

private:
	ShvJournalNode* m_node;
	shv::chainpack::RpcValue m_sinceParam;
	const shv::chainpack::RpcValue m_untilParam;
	std::vector<shv::core::utils::ShvJournalEntry> m_downloadedEntries;

	QString m_remoteLogShvPath;
	QString m_cacheDirPath;

	std::function<void(cp::RpcResponse::Error)> m_callback;
};

void ShvJournalNode::syncLog(const std::string& shv_path, const std::function<void(cp::RpcResponse::Error)> cb)
{
	if (m_logType == LogType::Legacy) {
		new LegacyFileSyncer(this, m_remoteLogShvPath, m_cacheDirPath, cb);
	} else {
		new FileSyncer(this, m_slaveHps, shv_path, m_cacheDirPath, cb);
	}
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

	if (method == "logSize") {
		return shv::chainpack::RpcValue::Int(calculateCacheDirSize());
	}

	if (method == "sanitizeLog") {
		sanitizeSize();
		return "Cache sanitization done";
	}

	return Super::callMethodRq(rq);
}

#include "shvjournalnode.moc"
