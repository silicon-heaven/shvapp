#include "application.h"
#include "logdirreader.h"

#include <QFile>

namespace cp = shv::chainpack;
using namespace shv::core::utils;

LogDirReader::LogDirReader(const std::string &path_prefix, const QDateTime &since, const QStringList &logs, shv::core::utils::ShvLogHeader &header)
	: m_pathPrefix(path_prefix)
	, m_logs(logs)
	, m_header(header)
	, m_logReader(nullptr)
	, m_journalReader(nullptr)
{
	m_previousFileUntil = since.isValid() ? since.toMSecsSinceEpoch() : 0LL;
	openNextFile();
}

LogDirReader::~LogDirReader()
{
	if (m_journalReader) {
		delete m_journalReader;
	}
	if (m_logReader) {
		delete m_logReader;
	}
}

bool LogDirReader::next()
{
	if (m_fakeEntryList.count()) {
		m_fakeEntryList.removeFirst();
	}
	if (m_fakeEntryList.count()) {
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

const ShvJournalEntry &LogDirReader::entry()
{
	if (m_fakeEntryList.count()) {
		return m_fakeEntryList[0];
	}
	if (m_journalReader) {
		return m_journalReader->entry();
	}
	else {
		return m_logReader->entry();
	}
}

void LogDirReader::openNextFile()
{
	if (m_journalReader) {
		delete m_journalReader;
		m_journalReader = nullptr;
	}
	if (m_logReader) {
		delete m_logReader;
		m_logReader = nullptr;
	}
	int64_t current_file_since = 0LL;
	bool cache_dirty_entry = false;
	if (m_logs.count() == 1) {
		if (QFile(m_logs[0]).exists()) {
			m_journalReader = new ShvJournalFileReader(m_logs[0].toStdString(), &m_header);
			if (m_journalReader->next()) {
				current_file_since = m_journalReader->entry().epochMsec;
				cache_dirty_entry = true;
			}
		}
	}
	else {
		m_logReader = new ShvLogFileReader(m_logs[0].toStdString());
		m_header = m_logReader->logHeader();
		current_file_since = m_header.since().toDateTime().msecsSinceEpoch();
	}

	if (!current_file_since || (current_file_since > Application::WORLD_BEGIN.toMSecsSinceEpoch() && m_previousFileUntil < current_file_since)) {
		m_fakeEntryList.push_back(ShvJournalEntry {
								   ShvJournalEntry::PATH_DATA_MISSING,
								   ShvJournalEntry::DATA_MISSING_LOG_FILE_MISSING,
								   ShvJournalEntry::DOMAIN_SHV_SYSTEM,
								   ShvJournalEntry::NO_SHORT_TIME,
								   ShvJournalEntry::SampleType::Continuous,
								   m_previousFileUntil
							   });
		if (current_file_since) {
			m_fakeEntryList.push_back(ShvJournalEntry {
									   ShvJournalEntry::PATH_DATA_MISSING,
									   "",
									   ShvJournalEntry::DOMAIN_SHV_SYSTEM,
									   ShvJournalEntry::NO_SHORT_TIME,
									   ShvJournalEntry::SampleType::Continuous,
									   current_file_since
								   });
		}
	}
	if (cache_dirty_entry) {
		m_fakeEntryList.push_back(m_journalReader->entry());
	}
	if (m_fakeEntryList.count()) {
		m_fakeEntryList.prepend(ShvJournalEntry());
	}

	if (m_logReader) {
		m_previousFileUntil = m_header.until().toDateTime().msecsSinceEpoch();
	}
}
