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
	LogDirReader(const QString &shv_path, bool is_push_log, int prefix_length, const QDateTime &since, const QDateTime &until, bool with_snapshot);
	LogDirReader(const LogDirReader &) = delete;
	~LogDirReader();

	bool next();
	const shv::core::utils::ShvJournalEntry &entry() const { return m_entry; }
	const std::string &pathPrefix() const { return m_pathPrefix; }
	const shv::core::utils::ShvLogTypeInfo &typeInfo() const { return m_typeInfo; }
	int64_t dirtyLogBegin() const { return m_dirtyLogBegin; }

private:
	void openNextFile();

	std::string m_pathPrefix;
	QStringList m_logs;
	QString m_dirtyLog;
	shv::core::utils::ShvLogHeader m_header;
	shv::core::utils::ShvLogFileReader *m_logReader;
	shv::core::utils::ShvJournalFileReader *m_journalReader;
	QVector<shv::core::utils::ShvJournalEntry> m_fakeEntryList;
	shv::core::utils::ShvLogTypeInfo m_typeInfo;
	int64_t m_previousFileUntil;
	int64_t m_until;
	shv::core::utils::ShvJournalEntry m_entry;
	bool m_firstFile;
	int64_t m_dirtyLogBegin;
	bool m_isPushLog;
};

#endif // LOGDIRREADER_H
