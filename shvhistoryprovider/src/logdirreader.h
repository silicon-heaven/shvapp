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
	LogDirReader(const QString &shv_path, int prefix_length, const QDateTime &since, const QDateTime &until);
	LogDirReader(const LogDirReader &) = delete;
	~LogDirReader();

	bool next();
	const shv::core::utils::ShvJournalEntry &entry() const { return m_entry; }
	const std::string &pathPrefix() const { return m_pathPrefix; }
	const shv::core::utils::ShvLogTypeInfo &typeInfo() const { return m_typeInfo; }
private:
	void openNextFile();

	std::string m_pathPrefix;
	QStringList m_logs;
	shv::core::utils::ShvLogHeader m_header;
	shv::core::utils::ShvLogFileReader *m_logReader;
	shv::core::utils::ShvJournalFileReader *m_journalReader;
	QVector<shv::core::utils::ShvJournalEntry> m_fakeEntryList;
	shv::core::utils::ShvLogTypeInfo m_typeInfo;
	int64_t m_previousFileUntil;
	shv::core::utils::ShvJournalEntry m_entry;
};

#endif // LOGDIRREADER_H
