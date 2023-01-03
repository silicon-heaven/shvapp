#include "leafnode.h"
#include "historyapp.h"
#include "appclioptions.h"
#include "shvjournalnode.h"
#include "getlog.h"
#include "utils.h"

#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/utils/shvlogrpcvaluereader.h>
#include <shv/core/utils/shvfilejournal.h>
#include <shv/coreqt/log.h>
#include <shv/coreqt/rpc.h>

#include <QDir>
#include <QDirIterator>
#include <QTimer>

#include <QCoroSignal>

#define journalDebug() shvCDebug("historyjournal")
#define journalInfo() shvCInfo("historyjournal")
#define journalWarning() shvCWarning("historyjournal")
#define journalError() shvCError("historyjournal")

namespace cp = shv::chainpack;
namespace {
static std::vector<cp::MetaMethod> methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE},
	{"getLog", cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{"logSize", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{"sanitizeLog", cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_WRITE},
};

static std::vector<cp::MetaMethod> push_log_methods {
	{"pushLog", cp::MetaMethod::Signature::VoidParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_WRITE},
};
}

using shv::core::Utils;

LeafNode::LeafNode(const std::string& node_id, LogType log_type, ShvNode* parent)
	: Super(node_id, parent)
	, m_journalCacheDir(Utils::joinPath(Utils::joinPath(HistoryApp::instance()->cliOptions()->journalCacheRoot(), shvPath()), std::string{"_shvjournal"}))
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

		std::string res_err;

		auto emit_pushlog_error = [&res_err] (const auto& msg) {
			journalWarning() << msg;
			res_err += msg;
			res_err += "\n";
		};

		shv::core::utils::ShvJournalFileWriter writer(file_path.toStdString());
		bool input_log_is_empty = true;
		journalDebug() << "Writing" << file_path;
		while (reader.next()) {
			input_log_is_empty = false;
			auto entry = reader.entry();
			if (entry.epochMsec < local_newest_entry_ms) {
				emit_pushlog_error("Rejecting push log entry for: " + shvPath() + " with timestamp: " + entry.dateTime().toIsoString() + " because a newer one already exists: " + local_newest_entry_str);
				continue;
			}

			if (entry.epochMsec == local_newest_entry_ms && std::find(local_newest_entry_paths.begin(), local_newest_entry_paths.end(), entry.path) != local_newest_entry_paths.end()) {
				emit_pushlog_error("Rejecting push log entry for: " + shvPath() + " with timestamp: " + entry.dateTime().toIsoString() + " and path: " + entry.path + " because we already have an entry with this timestamp and path");
				continue;
			}

			writer.append(reader.entry());
		}

		// The output file is always created by the writer. If we didn't manage to write any entries, we'll remove the
		// file.
		if (auto file = QFile(file_path); file.size() == 0) {
			file.remove();
		}

		if (input_log_is_empty) {
			return shv::chainpack::RpcValue::Map {
				{"since", shv::chainpack::RpcValue(nullptr)},
				{"until", shv::chainpack::RpcValue(nullptr)},
				{"msg", "Pushed log was empty"}
			};
		}

		return shv::chainpack::RpcValue::Map {
			{"since", reader.logHeader().since()},
			{"until", reader.logHeader().until()},
			{"msg", !res_err.empty() ? res_err : "success"}
		};
	}

	if (method == "getLog") {
		std::vector<std::function<shv::core::utils::ShvJournalFileReader()>> readers;
		auto journal_dir = QDir(QString::fromStdString(m_journalCacheDir));
		auto dir_files = journal_dir.entryList(QDir::Files, QDir::Name);
		for (const auto& file_name : dir_files) {
			readers.emplace_back([full_file_name = journal_dir.filePath(file_name).toStdString()] {
				return shv::core::utils::ShvJournalFileReader(full_file_name);
			});
		}

		return shv::core::utils::get_log(readers, shv::core::utils::ShvGetLogParams(params));
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
