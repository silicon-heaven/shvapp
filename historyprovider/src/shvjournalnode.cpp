#include "shvjournalnode.h"
#include "historyapp.h"
#include "appclioptions.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpccall.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/utils/shvlogrpcvaluereader.h>
#include <shv/core/utils/shvjournalentry.h>
#include <shv/core/utils/shvfilejournal.h>
#include <shv/core/utils/shvmemoryjournal.h>

#include <QDir>
#include <QDirIterator>
#include <QTimer>

#include <QCoroSignal>

#define journalDebug() shvCDebug("historyjournal")
#define journalInfo() shvCInfo("historyjournal")
#define journalError() shvCError("historyjournal")

namespace cp = shv::chainpack;
namespace {
static std::vector<cp::MetaMethod> methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{"isPushLog", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{"logSize", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{"sanitizeLog", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_WRITE},
	{"getLog", cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{"syncLog", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_WRITE},
};

static std::vector<cp::MetaMethod> methods_with_push_log {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{"isPushLog", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{"logSize", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{"sanitizeLog", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_WRITE},
	{"getLog", cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{"pushLog", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_WRITE},
};
}

ShvJournalNode::ShvJournalNode(const QString& site_shv_path, const QString& remote_log_shv_path, const LogType log_type)
	: Super(QString::fromStdString(shv::core::Utils::joinPath(HistoryApp::instance()->cliOptions()->journalCacheRoot(), site_shv_path.toStdString())), "shvjournal")
	, m_siteShvPath(site_shv_path.toStdString())
	, m_remoteLogShvPath(remote_log_shv_path)
	, m_cacheDirPath(QString::fromStdString(shv::core::Utils::joinPath(HistoryApp::instance()->cliOptions()->journalCacheRoot(), site_shv_path.toStdString())))
	, m_logType(log_type)
	, m_hasSyncLog(log_type != LogType::PushLog || m_remoteLogShvPath.indexOf(".local") != -1)
{
	QDir(m_cacheDirPath).mkpath(".");
	auto conn = HistoryApp::instance()->rpcConnection();
	if (log_type != LogType::PushLog) {
		connect(conn, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvJournalNode::onRpcMessageReceived);
		conn->callMethodSubscribe(site_shv_path.toStdString(), "chng");
	} else if (m_hasSyncLog) {
		auto tmr = new QTimer(this);
		connect(tmr, &QTimer::timeout, [this] { syncLog([] (auto /*error*/) { }); });
		tmr->start(HistoryApp::instance()->cliOptions()->logMaxAge() * 1000);
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

		if (path.find(m_siteShvPath) == 0 && method == "chng") {

			{
				auto writer = shv::core::utils::ShvJournalFileWriter(dirty_log_path(m_cacheDirPath));
				auto path_without_prefix = path.substr(m_siteShvPath.size());
				auto data_change = shv::chainpack::DataChange::fromRpcValue(ntf.params());

				auto entry = shv::core::utils::ShvJournalEntry(path_without_prefix, data_change.value()
														, shv::core::utils::ShvJournalEntry::DOMAIN_VAL_CHANGE
														, shv::core::utils::ShvJournalEntry::NO_SHORT_TIME
														, shv::core::utils::ShvJournalEntry::NO_VALUE_FLAGS
														, data_change.epochMSec());
				writer.append(entry);

			}

			shv::core::utils::ShvJournalFileReader reader(dirty_log_path(m_cacheDirPath));
			reader.next(); // There must be at least one entry, because I've just written it.
			if (QDateTime::currentMSecsSinceEpoch() - HistoryApp::instance()->cliOptions()->logMaxAge() * 1000 > reader.entry().epochMsec) {
				journalInfo() << "Log is too old, syncing journal" << m_remoteLogShvPath;
				syncLog([] (auto /*error*/) {
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

	return m_hasSyncLog ? methods.size() : methods_with_push_log.size();
}

const cp::MetaMethod* ShvJournalNode::metaMethod(const StringViewList& shv_path, size_t index)
{
	if (!shv_path.empty()) {
		// We'll let LocalFSNode handle the child nodes.
		return Super::metaMethod(shv_path, index);
	}

	return m_hasSyncLog ? &methods.at(index) : &methods_with_push_log.at(index);
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


void trim_dirty_log(const QString& cache_dir_path)
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
			journalDebug() << "Dirty logfile first entry timestamp" << reader.entry().epochMsec;
			first = false;
		}

		if (reader.entry().epochMsec >= newest_entry_msec) {
			new_dirty_log_entries.push_back(reader.entry());
		}
	}

	write_entries_to_file(QString::fromStdString(dirty_log_path(cache_dir_path)), new_dirty_log_entries);
}
}

class FileSyncer : public QObject {
	Q_OBJECT
public:
	FileSyncer(const cp::RpcValue& files, const QString& remote_log_shv_path, const QString& cache_dir_path, const std::function<void(cp::RpcResponse::Error)>& callback)
		: m_files(files)
		, m_filesList(m_files.asList())
		, m_remoteLogShvPath(remote_log_shv_path)
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
		for (const auto& currentFile : m_filesList) {
			auto file_name = QString::fromStdString(currentFile.asList().at(0).asString());
			if (file_name == "dirty.log2") {
				continue;
			}
			auto full_file_name = QDir(m_cacheDirPath).filePath(file_name);
			QFile file(full_file_name);
			auto local_size = file.size();
			auto remote_size = currentFile.asList().at(2).toInt();

			journalInfo() << "Syncing file" << full_file_name << "remote size:" << remote_size << "local size:" << (file.exists() ? QString::number(local_size) : "<doesn't exist>");
			if (file.exists()) {
				if (local_size == remote_size) {
					continue;
				}
			}

			auto sites_log_file = m_remoteLogShvPath + "/" + file_name;
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
		}

		writeFiles();
		trim_dirty_log(m_cacheDirPath);
		m_callback({});
		deleteLater();
	}

private:
	cp::RpcValue m_files;
	cp::RpcValue::List m_filesList;

	QMap<QString, cp::RpcValue> m_downloadedFiles;

	QString m_remoteLogShvPath;
	QString m_cacheDirPath;

	std::function<void(cp::RpcResponse::Error)> m_callback;
};

qint64 ShvJournalNode::calculateCacheDirSize() const
{
	journalDebug() << "Calculating cache directory size";
	QDirIterator iter(m_cacheDirPath, QDir::NoDotAndDotDot | QDir::Files);
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
	LegacyFileSyncer(const QString& remote_log_shv_path, const QString& cache_dir_path, const std::function<void(cp::RpcResponse::Error)>& callback)
		: m_untilParam(shv::chainpack::RpcValue::DateTime::now())
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
		}

		connect(this, &LegacyFileSyncer::chunkDone, this, &LegacyFileSyncer::downloadNextChunk);
		downloadNextChunk();
	}

	Q_SIGNAL void chunkDone();

	void writeEntriesToFile()
	{
		auto file_name = QDir(m_cacheDirPath).filePath(QString::fromStdString(shv::core::utils::ShvJournalFileReader::msecToBaseFileName(m_downloadedEntries.front().epochMsec)) + ".log2");

		write_entries_to_file(file_name, m_downloadedEntries);
		trim_dirty_log(m_cacheDirPath);
		m_callback({});
		deleteLater();
	}

	void downloadNextChunk()
	{
		shv::core::utils::ShvGetLogParams get_log_params;
		get_log_params.recordCountLimit = RECORD_COUNT_LIMIT;
		get_log_params.since = m_sinceParam;
		get_log_params.until = m_untilParam;

		auto call = shv::iotqt::rpc::RpcCall::create(HistoryApp::instance()->rpcConnection())
			->setShvPath(m_remoteLogShvPath)
			->setMethod("getLog")
			->setParams(get_log_params.toRpcValue());

		connect(call, &shv::iotqt::rpc::RpcCall::error, [this] (const QString& error) {
			journalError() << error;
			m_callback(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, "Couldn't retrieve logs from the device"));
			deleteLater();
		});

		connect(call, &shv::iotqt::rpc::RpcCall::result, [this] (const cp::RpcValue& result) {
			shv::core::utils::ShvMemoryJournal result_log;
			result_log.loadLog(result);
			result_log.clearSnapshot();
			if (result_log.isEmpty()) {
				writeEntriesToFile();
				return;
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
				return;
			}

			m_sinceParam = remote_entries.back().dateTime();
			emit chunkDone();

		});
		call->start();

	}

private:
	shv::chainpack::RpcValue m_sinceParam;
	const shv::chainpack::RpcValue m_untilParam;
	std::vector<shv::core::utils::ShvJournalEntry> m_downloadedEntries;

	QString m_remoteLogShvPath;
	QString m_cacheDirPath;

	std::function<void(cp::RpcResponse::Error)> m_callback;
};

void ShvJournalNode::syncLog(const std::function<void(cp::RpcResponse::Error)> cb)
{
	if (m_logType == LogType::Legacy) {
		new LegacyFileSyncer(m_remoteLogShvPath, m_cacheDirPath, cb);
	} else {
		auto call = shv::iotqt::rpc::RpcCall::create(HistoryApp::instance()->rpcConnection())
			->setShvPath(m_remoteLogShvPath)
			->setMethod("lsfiles")
			->setParams(cp::RpcValue::Map{{"size", true}});

		connect(call, &shv::iotqt::rpc::RpcCall::error, [cb] (const QString& error) {
			journalError() << error;
			cb(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, "Couldn't retrieve filelist from the device"));
		});

		connect(call, &shv::iotqt::rpc::RpcCall::result, [this, cb] (const cp::RpcValue& result) {
			journalDebug() << "Got filelist from" << m_remoteLogShvPath;
			new FileSyncer(result, m_remoteLogShvPath, m_cacheDirPath, cb);
		});
		call->start();
	}
}

cp::RpcValue ShvJournalNode::callMethodRq(const cp::RpcRequest &rq)
{
	auto method = rq.method().asString();
	if (method == "syncLog" && m_hasSyncLog) {
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

		syncLog(respond);

		return {};
	}

	if (method == "pushLog" && m_logType == LogType::PushLog) {
		journalInfo() << "Got pushLog at" << m_siteShvPath;

		shv::core::utils::ShvLogRpcValueReader reader(rq.params());
		QDir cache_dir(m_cacheDirPath);
		auto remote_log_time_ms = reader.logHeader().dateTimeCRef().toDateTime().msecsSinceEpoch();
		auto entries = cache_dir.entryList(QDir::NoDotAndDotDot | QDir::Files, QDir::Name | QDir::Reversed);
		if (!std::empty(entries)) {
			auto local_newest_log_time = entries.at(0).toStdString();

			if (shv::core::utils::ShvJournalFileReader::fileNameToFileMsec(local_newest_log_time) >= remote_log_time_ms) {
				auto remote_timestamp = shv::core::utils::ShvJournalFileReader::msecToBaseFileName(remote_log_time_ms);
				auto err_message = "Rejecting push log with timestamp: " + remote_timestamp + ", because a newer or same already exists: " + local_newest_log_time;
				journalError() << err_message;
				throw shv::core::Exception(err_message);
			}
		}

		auto file_path = cache_dir.filePath(QString::fromStdString(shv::core::utils::ShvJournalFileReader::msecToBaseFileName(remote_log_time_ms)) + ".log2");

		shv::core::utils::ShvJournalFileWriter writer(file_path.toStdString());
		journalDebug() << "Writing" << file_path;
		while (reader.next()) {
			writer.append(reader.entry());
		}
		return true;
	}

	if (method == "getLog") {
		shv::core::utils::ShvFileJournal file_journal("historyprovider");
		file_journal.setJournalDir(m_cacheDirPath.toStdString());
		auto get_log_params = shv::core::utils::ShvGetLogParams(rq.params());
		return file_journal.getLog(get_log_params);
	}

	if (method == "isPushLog") {
		return m_logType == LogType::PushLog;
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
