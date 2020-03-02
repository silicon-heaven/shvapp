#include "application.h"
#include "logdir.h"
#include "logdirreader.h"

#include <QFile>

namespace cp = shv::chainpack;
using namespace shv::core::utils;

LogDirReader::LogDirReader(const QString &shv_path, int prefix_length, const QDateTime &since, const QDateTime &until)
	: m_logReader(nullptr)
	, m_journalReader(nullptr)
{
	LogDir log_dir(shv_path);
	m_logs = log_dir.findFiles(since, until);
	if (m_logs.count() == 0) {
		QStringList all_logs = log_dir.findFiles(QDateTime(), QDateTime());
		if (all_logs.count()) {
			ShvLogFileReader log_reader(all_logs.last().toStdString());
			m_header = log_reader.logHeader();
		}
	}
	m_logs << log_dir.dirtyLogPath();

	if (prefix_length) {
		m_pathPrefix = (shv_path.right(prefix_length - 1) + "/").toStdString();
	}

	m_previousFileUntil = since.isValid() ? since.toMSecsSinceEpoch() : 0LL;
	m_typeInfo = m_header.typeInfo();
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
		m_entry = m_fakeEntryList[0];
		m_fakeEntryList.removeFirst();
		return true;
	}
	bool has_next = false;
	if (m_journalReader) {
		has_next = m_journalReader->next();
		m_entry = m_journalReader->entry();
	}
	else if (m_logReader){
		has_next = m_logReader->next();
		m_entry = m_logReader->entry();
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
		if (!QFile(m_logs[0]).exists()) {
			m_logs.removeFirst();
			openNextFile();
			return;
		}
		else {
			m_logReader = new ShvLogFileReader(m_logs[0].toStdString());
			m_header = m_logReader->logHeader();
			current_file_since = m_header.since().toDateTime().msecsSinceEpoch();
			m_typeInfo = m_header.typeInfo();
		}
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
	if (m_logReader) {
		m_previousFileUntil = m_header.until().toDateTime().msecsSinceEpoch();
	}
}
