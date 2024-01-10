#include "leafnode.h"
#include "historyapp.h"
#include "appclioptions.h"
#include "utils.h"

#include <shv/core/utils/getlog.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/utils/shvlogrpcvaluereader.h>
#include <shv/core/utils/shvfilejournal.h>
#include <shv/coreqt/log.h>
#include <shv/coreqt/rpc.h>

#include <QDir>
#include <QDirIterator>
#include <QTimer>

#define journalDebug() shvCDebug("historyjournal")
#define journalInfo() shvCInfo("historyjournal")

namespace cp = shv::chainpack;
namespace {
const std::vector<cp::MetaMethod> methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE},
	{"getLog", cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{"logSize", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{"sanitizeLog", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_WRITE},
};

const std::vector<cp::MetaMethod> push_log_methods {
	{"pushLog", cp::MetaMethod::Signature::VoidParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_WRITE},
};
}

LeafNode::LeafNode(const std::string& node_id, const std::string& journal_cache_dir, LogType log_type, ShvNode* parent)
	: Super(node_id, parent)
	, m_journalCacheDir(journal_cache_dir)
	, m_logType(log_type)
{
	QDir(QString::fromStdString(m_journalCacheDir)).mkpath(".");
}

size_t LeafNode::methodCount(const StringViewList& shv_path)
{
	if (shv_path.empty()) {
		if (m_logType == LogType::PushLog) {
			return methods.size() + push_log_methods.size();
		}

		return methods.size();

	}

	return Super::methodCount(shv_path);
}

const cp::MetaMethod* LeafNode::metaMethod(const StringViewList& shv_path, size_t index)
{
	if (shv_path.empty()) {
		if (index >= methods.size()) {
			return &push_log_methods.at(index - methods.size());
		}

		return &methods.at(index);
	}

	return Super::metaMethod(shv_path, index);
}

qint64 LeafNode::calculateCacheDirSize() const
{
	journalDebug() << "Calculating cache directory size";
	QDirIterator iter(QString::fromStdString(m_journalCacheDir), QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories);
	qint64 total_size = 0;
	while (iter.hasNext()) {
		QFile file(iter.next());
		total_size += file.size();
	}
	journalDebug() << "Cache directory size" << total_size;

	return total_size;
}

void LeafNode::sanitizeSize()
{
	auto cache_dir_size = calculateCacheDirSize();
	auto cache_size_limit = HistoryApp::instance()->singleCacheSizeLimit();

	journalInfo() << "Sanitizing cache, path:" << m_journalCacheDir << "size" << cache_dir_size << "cacheSizeLimit" << cache_size_limit;

	QDir cache_dir(QString::fromStdString(m_journalCacheDir));
	auto entries = cache_dir.entryList(QDir::NoDotAndDotDot | QDir::Files, QDir::Name);
	QStringListIterator iter(entries);

	while (cache_dir_size > cache_size_limit && iter.hasNext()) {
		QFile file(cache_dir.filePath(iter.next()));
		journalDebug() << "Removing" << file.fileName();
		cache_dir_size -= file.size();
		file.remove();
	}

	journalInfo() << "Sanitization done, path:" << m_journalCacheDir << "new size" << cache_dir_size << "cacheSizeLimit" << cache_size_limit;
}

shv::chainpack::RpcValue LeafNode::callMethod(const StringViewList& shv_path, const std::string& method, const shv::chainpack::RpcValue& params, const shv::chainpack::RpcValue& user_id)
{
	if (method == "pushLog" && m_logType == LogType::PushLog) {
		journalInfo() << "Got pushLog at" << shvPath();

		shv::core::utils::ShvLogRpcValueReader reader(params);
		QDir cache_dir(QString::fromStdString(m_journalCacheDir));
		auto remote_since_ms = reader.logHeader().sinceCRef().toDateTime().msecsSinceEpoch();
		int64_t local_newest_entry_ms = 0;
		std::string local_newest_entry_str;
		std::vector<std::string> local_newest_entry_paths;
		auto entries = cache_dir.entryList(QDir::NoDotAndDotDot | QDir::Files, QDir::Name | QDir::Reversed);
		if (!std::empty(entries)) {
			auto local_newest_log_file = entries.at(0);
			auto newest_file_entries = read_entries_from_file(cache_dir.filePath(local_newest_log_file));
			if (!newest_file_entries.empty()) {
				local_newest_entry_ms = newest_file_entries.back().dateTime().msecsSinceEpoch();
				local_newest_entry_str = newest_file_entries.back().dateTime().toIsoString();
				for (const auto& entry : newest_file_entries) {
					if (entry.epochMsec == local_newest_entry_ms) {
						local_newest_entry_paths.push_back(entry.path);
					}
				}
			}
		}

		auto file_path = cache_dir.filePath(QString::fromStdString(shv::core::utils::ShvJournalFileReader::msecToBaseFileName(remote_since_ms)) + ".log2");

		shv::core::utils::ShvJournalFileWriter writer(file_path.toStdString());
		int remote_entries_count = 0;
		int written_entries_count = 0;
		journalDebug() << "Writing" << file_path;
		while (reader.next()) {
			auto entry = reader.entry();
			remote_entries_count++;
			if (entry.epochMsec < local_newest_entry_ms) {
				shvWarning() << "Rejecting push log entry for:" << shvPath() << "with timestamp:" << entry.dateTime().toIsoString() << "because a newer one already exists:" << local_newest_entry_str;
				continue;
			}

			if (entry.epochMsec == local_newest_entry_ms && std::find(local_newest_entry_paths.begin(), local_newest_entry_paths.end(), entry.path) != local_newest_entry_paths.end()) {
				shvWarning() << "Rejecting push log entry for:" << shvPath() << "with timestamp:" << entry.dateTime().toIsoString() << "and path:" << entry.path << "because we already have an entry with this timestamp and path";
				continue;
			}

			writer.append(reader.entry());
			written_entries_count++;
		}

		journalInfo() << "Got" << remote_entries_count << "entries (" + std::to_string(remote_entries_count - written_entries_count) + " were rejected)" << "from pushLog at" << shvPath();

		// The output file is always created by the writer. If we didn't manage to write any entries, we'll remove the
		// file.
		if (auto file = QFile(file_path); file.size() == 0) {
			file.remove();
		}

		if (remote_entries_count == 0) {
			return shv::chainpack::RpcValue::Map {
				{"since", shv::chainpack::RpcValue(nullptr)},
				{"until", shv::chainpack::RpcValue(nullptr)},
				{"msg", "success"}
			};
		}

		return shv::chainpack::RpcValue::Map {
			{"since", reader.logHeader().since()},
			{"until", reader.logHeader().until()},
			{"msg", "success"}
		};
	}

	if (method == "getLog") {
		std::vector<std::function<shv::core::utils::ShvJournalFileReader()>> readers;
		auto journal_dir = QDir(QString::fromStdString(m_journalCacheDir));
		auto dir_files = journal_dir.entryList(QDir::Files, QDir::Name);
		shv::core::utils::ShvGetLogParams get_log_params(params);
		auto newest_matching_file_it = [&get_log_params, &dir_files] {
			// If there's no since param, return everything.
			if ((!get_log_params.since.isDateTime() && !get_log_params.isSinceLast()) || dir_files.begin() == dir_files.end()) {
				return dir_files.begin();
			}

			// If the first file is the dirtylog, just return it. There are no other files.
			if (*dir_files.begin() == "dirtylog") {
				return dir_files.begin();
			}

			// For since == last, we need to take the dirtylog and the last file.
			// the previous condition would catch that.
			if (get_log_params.isSinceLast()) {
				// There is at least one file, let's look at it. If it's the dirty log, then there must be at least one
				// more file, because the previous condition checks whether there is only the dirty log.
				auto it = dir_files.end() - 1;
				if (*it != "dirtylog") {
					// There's no dirtylog, so we'll just return the last file.
					return it;
				}

				// Otherwise, there's a dirtylog and we'll return the last file AND the dirtylog.
				return dir_files.end() - 2;
			}

			// If the first file is newer than since, than just return it.
			auto since_param_ms = get_log_params.since.toDateTime().msecsSinceEpoch();
			if (shv::core::utils::ShvJournalFileReader::fileNameToFileMsec(dir_files.begin()->toStdString()) >= since_param_ms) {
				return dir_files.begin();
			}

			// Otherwise the first file is older than since.
			// Try to find files that are newer, but still older than since.
			auto last_matching_it = dir_files.begin();
			for (auto it = dir_files.begin(); it != dir_files.end(); ++it) {
				if (*it == "dirtylog" || shv::core::utils::ShvJournalFileReader::fileNameToFileMsec(it->toStdString()) >= since_param_ms) {
					break;
				}

				last_matching_it = it;
			}

			return last_matching_it;
		}();

		for (auto it = newest_matching_file_it; it != dir_files.end(); ++it) {
			journalDebug() << "Using" << *it << "for getLog at" << shvPath();
			readers.emplace_back([full_file_name = journal_dir.filePath(*it).toStdString()] {
				return shv::core::utils::ShvJournalFileReader(full_file_name);
			});
		}

		return shv::core::utils::getLog(readers, get_log_params);
	}

	if (method == "logSize") {
		return shv::chainpack::RpcValue::Int(calculateCacheDirSize());
	}

	if (method == "sanitizeLog") {
		sanitizeSize();
		return "Cache sanitization done";
	}

	return Super::callMethod(shv_path, method, params, user_id);
}
