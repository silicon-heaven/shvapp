#include "shvjournalnode.h"
#include "historyapp.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpccall.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/utils/shvjournalentry.h>

#include <QDir>
#include <QTimer>

#define journalDebug() shvCDebug("historyjournal")
#define journalInfo() shvCInfo("historyjournal")
#define journalError() shvCError("historyjournal")

namespace cp = shv::chainpack;
namespace {
std::vector<cp::MetaMethod> methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{"syncLog", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_WRITE},
};
}

ShvJournalNode::ShvJournalNode(const QString& site_path)
	: Super(QString::fromStdString(shv::core::Utils::joinPath(std::string("/tmp/historyprovider"), site_path.toStdString())), "shvjournal")
	, m_sitePath(site_path.toStdString())
	, m_logsPath(QString::fromStdString(shv::core::Utils::joinPath(site_path.toStdString(), std::string{".app/shvjournal"})))
	, m_repoPath(QString::fromStdString(shv::core::Utils::joinPath(std::string("/tmp/historyprovider"), site_path.toStdString())))
{
	QDir(m_repoPath).mkpath(".");
	auto conn = HistoryApp::instance()->rpcConnection();
	connect(conn, &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &ShvJournalNode::onRpcMessageReceived);
	conn->callMethodSubscribe(site_path.toStdString(), "chng");
}

namespace {
const auto DIRTY_FILENAME = "dirty.log2";

std::string dirty_log_path(const QString& repo_path)
{
	return QDir(repo_path).filePath(DIRTY_FILENAME).toStdString();
}
}

void ShvJournalNode::onRpcMessageReceived(const cp::RpcMessage &msg)
{
	if (msg.isSignal()) {
		cp::RpcSignal ntf(msg);
		auto path = ntf.shvPath().asString();
		auto method = ntf.method().asString();
		auto value = ntf.value();

		if (path.find(m_sitePath) == 0 && method == "chng") {
			auto writer = shv::core::utils::ShvJournalFileWriter(dirty_log_path(m_repoPath));
			auto path_without_prefix = path.substr(m_sitePath.size());
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

size_t ShvJournalNode::methodCount(const StringViewList& shv_path)
{
	if (!shv_path.empty()) {
		// We'll let LocalFSNode handle the child nodes.
		return Super::methodCount(shv_path);
	}

	return methods.size();
}

const cp::MetaMethod* ShvJournalNode::metaMethod(const StringViewList& shv_path, size_t index)
{
	if (!shv_path.empty()) {
		// We'll let LocalFSNode handle the child nodes.
		return Super::metaMethod(shv_path, index);
	}

	return &methods.at(index);
}

class FileSyncer : public QObject {
	Q_OBJECT;
public:
	FileSyncer(const cp::RpcValue& files, const QString& logs_path, const QString& repo_path, const cp::RpcRequest& request)
		: m_files(files)
		, m_filesList(m_files.asList())
		, m_currentFile(m_filesList.begin())
		, m_logsPath(logs_path)
		, m_repoPath(repo_path)
		, m_request(request)
	{
		connect(this, &FileSyncer::fileDone, [this] {
			++m_currentFile;
			syncCurrentFile();
		});

		syncCurrentFile();
	}

	void trimDirtyLog()
	{
		QDir repo_dir(m_repoPath);
		auto entries = repo_dir.entryList(QDir::NoDotAndDotDot | QDir::Files, QDir::Name | QDir::Reversed);

		if (entries.empty() || entries.at(0) != DIRTY_FILENAME) {
			journalDebug() << "No dirty log, nothing to trim";
			return;
		}

		if (entries.size() == 1) {
			journalDebug() << "No logs to use for trimming";
			return;
		}

		if (!shv::core::utils::ShvJournalFileReader(dirty_log_path(m_repoPath)).next()) {
			// No entries in the dirty log file, nothing to change.
			journalDebug() << "Dirty log empty, nothing to trim";
			return;
		}

		auto newest_log_filepath = repo_dir.filePath(entries.at(1));

		// It is possible that the newest logfile contains multiple entries with the same timestamp. Because it could
		// have been written (by shvagent) in between two events that happenede in the same millisecond, we can't be
		// sure whether we have all events from the last millisecond. Because of that we will discard the last
		// millisecond from the synced log, and keep it in the dirty log.
		std::vector<shv::core::utils::ShvJournalEntry> newest_log_entries;
		shv::core::utils::ShvJournalFileReader reader(newest_log_filepath.toStdString());
		int64_t newest_entry_msec = 0;
		while (reader.next()) {
			newest_log_entries.push_back(reader.entry());
			newest_entry_msec = reader.entry().epochMsec;
		}

		journalDebug() << "Newest logfile" << newest_log_filepath << "last entry timestamp" << newest_entry_msec << "entry count" << newest_log_entries.size();
		QFile(newest_log_filepath).remove();
		shv::core::utils::ShvJournalFileWriter writer(newest_log_filepath.toStdString());
		for (const auto& entry : newest_log_entries) {
			if (entry.epochMsec == newest_entry_msec) {
				// We don't want the last millisecond.
				break;
			}
			writer.append(entry);
		}

		newest_entry_msec = writer.recentTimeStamp();
		journalDebug() << "Trimmed logfile" << newest_log_filepath << "last entry timestamp" << newest_entry_msec;

		// Now filter dirty log's newer events.
		reader = shv::core::utils::ShvJournalFileReader(dirty_log_path(m_repoPath));
		std::vector<shv::core::utils::ShvJournalEntry> new_dirty_log_entries;
		bool first = true;
		while (reader.next()) {
			if (first) {
				journalDebug() << "Dirty logfile first entry timestamp" << reader.entry().epochMsec;
				first = false;
			}

			if (reader.entry().epochMsec > newest_entry_msec) {
				new_dirty_log_entries.push_back(reader.entry());
			}
		}

		QFile(QString::fromStdString(dirty_log_path(m_repoPath))).remove();

		writer = shv::core::utils::ShvJournalFileWriter(dirty_log_path(m_repoPath));
		for (const auto& entry : new_dirty_log_entries) {
			writer.append(entry);
		}
	}

	Q_SLOT void syncCurrentFile()
	{
		if (m_currentFile == m_filesList.end()) {
			trimDirtyLog();
			auto response = m_request.makeResponse();
			response.setResult("All files have been synced");
			HistoryApp::instance()->rpcConnection()->sendMessage(response);
			deleteLater();
			return;
		}

		auto file_name = QString::fromStdString(m_currentFile->asList().at(0).asString());
		auto full_file_name = QDir(m_repoPath).filePath(file_name);
		QFile file(full_file_name);
		auto local_size = file.size();
		auto remote_size = m_currentFile->asList().at(2).toInt();

		journalInfo() << "Syncing file" << full_file_name << "remote size:" << remote_size << "local size:" << (file.exists() ? QString::number(local_size) : "<doesn't exist>");
		if (file.exists()) {
			if (local_size == remote_size) {
				emit fileDone();
				return;
			}
		}

		auto sites_log_file = m_logsPath + "/" + file_name;
		journalDebug() << "Retrieving" << sites_log_file << "offset:" << local_size;
		auto call = shv::iotqt::rpc::RpcCall::create(HistoryApp::instance()->rpcConnection())
			->setShvPath(sites_log_file)
			->setMethod("read")
			->setParams(cp::RpcValue::Map{{"offset", cp::RpcValue::Int(local_size)}});
		connect(call, &shv::iotqt::rpc::RpcCall::error, [sites_log_file] (const QString& error) {
			journalError() << "Couldn't retrieve" << sites_log_file << ":" << error;
		});

		connect(call, &shv::iotqt::rpc::RpcCall::result, [this, full_file_name, sites_log_file] (const cp::RpcValue& result) {
			journalDebug() << "Got" << sites_log_file;
			QFile log_file(full_file_name);
			if (log_file.open(QFile::WriteOnly | QFile::Append)) {
				auto blob = result.asBlob();
				journalDebug() << "Writing" << full_file_name;
				log_file.write(reinterpret_cast<const char*>(blob.data()), blob.size());
			} else {
				journalError() << "Couldn't open" << full_file_name << "for writing";
			}
			emit fileDone();
		});
		call->start();
	}

    Q_SIGNAL void fileDone();

private:
	cp::RpcValue m_files;
	cp::RpcValue::List m_filesList;
	cp::RpcValue::List::const_iterator m_currentFile;

	QString m_logsPath;
	QString m_repoPath;

	cp::RpcRequest m_request;

};

cp::RpcValue ShvJournalNode::callMethodRq(const cp::RpcRequest &rq)
{
	auto method = rq.method().asString();
	if (method == "syncLog") {
		journalInfo() << "Syncing shvjournal" << m_logsPath;
		auto call = shv::iotqt::rpc::RpcCall::create(HistoryApp::instance()->rpcConnection())
			->setShvPath(m_logsPath)
			->setMethod("ls")
			->setParams(cp::RpcValue::Map{{"size", true}});

		connect(call, &shv::iotqt::rpc::RpcCall::error, [rq] (const QString& error) {
			journalError() << error;
			auto response = rq.makeResponse();
			response.setError(cp::RpcResponse::Error::create(
						cp::RpcResponse::Error::MethodCallException, "Couldn't retrieve filelist from the device"));
			HistoryApp::instance()->rpcConnection()->sendMessage(response);
		});

		connect(call, &shv::iotqt::rpc::RpcCall::result, [this, rq] (const cp::RpcValue& result) {
			journalDebug() << "Got filelist from" << m_logsPath;
			new FileSyncer(result, m_logsPath, m_repoPath, rq);
		});
		call->start();

		return {};
	}

	return Super::callMethodRq(rq);
}

#include "shvjournalnode.moc"
