#include "application.h"
#include "logdir.h"
#include "devicemonitor.h"
#include "getlogmerge.h"
#include "siteitem.h"

#include <QDateTime>

#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvlogfilereader.h>
#include <shv/coreqt/exception.h>
#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/rpc.h>

namespace cp = shv::chainpack;
using namespace shv::core::utils;

class LogDirReader
{
public:
	LogDirReader(const std::string &path_prefix, const QDateTime &since, const QStringList &logs, ShvLogHeader &header)
		: m_pathPrefix(path_prefix)
		, m_logs(logs)
		, m_header(header)
		, m_logReader(nullptr)
		, m_journalReader(nullptr)
	{
		m_requestedDataBegin = cp::RpcValue::fromValue<QDateTime>(since);
		openNextFile();
	}

	LogDirReader(const LogDirReader &) = delete;

	~LogDirReader()
	{
		if (m_journalReader) {
			delete m_journalReader;
		}
		if (m_logReader) {
			delete m_logReader;
		}
	}

	bool next()
	{
		if (m_entryCache.count()) {
			m_entryCache.removeFirst();
		}
		if (m_entryCache.count()) {
			return true;
		}
		bool has_next = false;
		if (m_journalReader) {
			has_next = m_journalReader->next();
		}
		else if (m_logReader){
			has_next = m_logReader->next();
		}
		if (has_next) {
			return true;
		}
		if (m_logs.count() == 1) {
			return false;
		}
		m_logs.removeFirst();
		openNextFile();
		return next();
	}

	const ShvJournalEntry &entry()
	{
		if (m_entryCache.count()) {
			return m_entryCache[0];
		}
		if (m_journalReader) {
			return m_journalReader->entry();
		}
		else {
			return m_logReader->entry();
		}
	}

	const std::string &pathPrefix() const { return m_pathPrefix; }

private:
	void openNextFile()
	{
		if (m_journalReader) {
			delete m_journalReader;
			m_journalReader = nullptr;
		}
		if (m_logReader) {
			delete m_logReader;
			m_logReader = nullptr;
		}
		cp::RpcValue found_data_begin;
		bool cache_dirty_entry = false;
		if (m_logs.count() == 1) {
			if (QFile(m_logs[0]).exists()) {
				m_journalReader = new ShvJournalFileReader(m_logs[0].toStdString(), &m_header);
				if (m_journalReader->next()) {
					found_data_begin = cp::RpcValue::DateTime::fromMSecsSinceEpoch(m_journalReader->entry().epochMsec);
					cache_dirty_entry = true;
				}
			}
		}
		else {
			m_logReader = new ShvLogFileReader(m_logs[0].toStdString());
			m_header = m_logReader->logHeader();
			found_data_begin = m_header.since();
			if (!m_requestedDataBegin.isDateTime() || m_requestedDataBegin.toDateTime() < found_data_begin.toDateTime()) {
				m_entryCache.push_back(ShvJournalEntry {
										   ShvJournalEntry::PATH_DATA_MISSING,
										   ShvJournalEntry::DATA_MISSING_LOG_FILE_MISSING,
										   ShvJournalEntry::DOMAIN_SHV_SYSTEM,
										   ShvJournalEntry::NO_SHORT_TIME,
										   ShvJournalEntry::SampleType::Continuous,
										   m_requestedDataBegin.isDateTime() ? m_requestedDataBegin.toDateTime().msecsSinceEpoch() : 0
									   });
				if (found_data_begin.isDateTime()) {
					m_entryCache.push_back(ShvJournalEntry {
											   ShvJournalEntry::PATH_DATA_MISSING,
											   "",
											   ShvJournalEntry::DOMAIN_SHV_SYSTEM,
											   ShvJournalEntry::NO_SHORT_TIME,
											   ShvJournalEntry::SampleType::Continuous,
											   found_data_begin.toDateTime().msecsSinceEpoch()
										   });
					if (cache_dirty_entry) {
						m_entryCache.push_back(m_journalReader->entry());
					}
				}
			}
		}
		if (m_logReader) {
			m_requestedDataBegin = m_header.until();
		}
	}

	std::string m_pathPrefix;
	QStringList m_logs;
	ShvLogHeader m_header;
	ShvLogFileReader *m_logReader;
	ShvJournalFileReader *m_journalReader;
	QVector<ShvJournalEntry> m_entryCache;
	cp::RpcValue m_requestedDataBegin;
};

GetLogMerge::GetLogMerge(const QString &shv_path, const shv::core::utils::ShvGetLogParams &log_params)
	: m_shvPath(shv_path)
	, m_logParams(log_params)
	, m_mergedLog(log_params)
{
	const SiteItem *site_item = Application::instance()->deviceMonitor()->sites()->itemByShvPath(shv_path);
	if (!site_item) {
		SHV_EXCEPTION("Invalid shv_path");
	}
	if (qobject_cast<const SitesHPDevice *>(site_item)) {
		m_shvPaths << m_shvPath;
	}
	else {
		for (const SitesHPDevice *device : site_item->findChildren<const SitesHPDevice*>()) {
			m_shvPaths << device->shvPath();
		}
	}
}

shv::core::utils::ShvMemoryJournal &GetLogMerge::getLog()
{
	QDateTime since = rpcvalue_cast<QDateTime>(m_logParams.since);
	QDateTime until = rpcvalue_cast<QDateTime>(m_logParams.until);

	QVector<LogDirReader*> readers;
	for (const QString &shv_path : m_shvPaths) {
		LogDir log_dir(shv_path);
		QStringList logs = log_dir.findFiles(since, until);
		ShvLogHeader header;
		if (logs.count() == 0) {
			QStringList all_logs = log_dir.findFiles(QDateTime(), QDateTime());
			if (all_logs.count()) {
				ShvLogFileReader log_reader(all_logs.last().toStdString());
				header = log_reader.logHeader();
			}
		}
		logs << log_dir.dirtyLogPath();

		QString path_prefix;
		int prepend_path_length = shv_path.length() - m_shvPath.length();
		if (prepend_path_length) {
			path_prefix = shv_path.right(prepend_path_length - 1) + "/";
		}

		LogDirReader *reader = new LogDirReader(path_prefix.toStdString(), since, logs, header);
		if (reader->next()) {
			readers << reader;
		}
		else {
			delete reader;
		}
	}
	int64_t until_msecs = until.isNull() ? std::numeric_limits<int64_t>::max() : until.toMSecsSinceEpoch();

	while (readers.count()) {
		int64_t oldest = std::numeric_limits<int64_t>::max();
		int oldest_index = -1;
		for (int i = 0; i < readers.count(); ++i) {
			if (readers[i]->entry().epochMsec < oldest) {
				oldest = readers[i]->entry().epochMsec;
				oldest_index = i;
			}
		}
		LogDirReader *reader = readers[oldest_index];
		ShvJournalEntry entry = reader->entry();
		if (entry.epochMsec >= until_msecs) {
			break;
		}
		entry.path = reader->pathPrefix() + entry.path;
		m_mergedLog.append(entry);
		if ((int)m_mergedLog.entries().size() >= m_mergedLog.inputFilterRecordCountLimit()) {
			break;
		}
		if (!reader->next()) {
			delete reader;
			readers.removeAt(oldest_index);
		}
	}

	return m_mergedLog;
}
