#ifndef LOGDIRREADER_H
#define LOGDIRREADER_H

#include <shv/core/utils/shvjournalentry.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvlogfilereader.h>
#include <shv/core/utils/shvlogheader.h>

#include <QDateTime>
#include <QVector>

class LogDirReader
{
public:
	LogDirReader(const std::string &path_prefix, const QDateTime &since, const QStringList &logs, shv::core::utils::ShvLogHeader &header);
	LogDirReader(const LogDirReader &) = delete;
	~LogDirReader();

	bool next();
	const shv::core::utils::ShvJournalEntry &entry();
	const std::string &pathPrefix() const { return m_pathPrefix; }

private:
	void openNextFile();

	std::string m_pathPrefix;
	QStringList m_logs;
	shv::core::utils::ShvLogHeader m_header;
	shv::core::utils::ShvLogFileReader *m_logReader;
	shv::core::utils::ShvJournalFileReader *m_journalReader;
	QVector<shv::core::utils::ShvJournalEntry> m_fakeEntryList;
	int64_t m_previousFileUntil;
};

#endif // LOGDIRREADER_H
