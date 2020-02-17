#include "application.h"
#include "logdir.h"
#include "devicemonitor.h"
#include "getlogmerge.h"

#include <QDateTime>

#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvlogfilereader.h>
#include <shv/coreqt/exception.h>
#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/rpc.h>

namespace cp = shv::chainpack;
using namespace shv::core::utils;

GetLogMerge::GetLogMerge(const QString &shv_path, const shv::core::utils::ShvGetLogParams &log_params)
	: m_shvPath(shv_path)
	, m_logParams(log_params)
	, m_mergedLog(log_params)
{
}

shv::core::utils::ShvMemoryJournal &GetLogMerge::getLog()
{
	QDateTime since = rpcvalue_cast<QDateTime>(m_logParams.since);
	QDateTime until = rpcvalue_cast<QDateTime>(m_logParams.until);

	LogDir log_dir(m_shvPath);
	QStringList logs = log_dir.findFiles(since, until);
	int64_t until_msecs = until.isNull() ? std::numeric_limits<int64_t>::max() : until.toMSecsSinceEpoch();

	bool completed = false;

	ShvLogHeader header;
	for (const QString &log : logs) {

		ShvLogFileReader log_reader(log.toStdString());
		header = log_reader.logHeader();
		while (log_reader.next()) {
			const ShvJournalEntry &entry = log_reader.entry();
			if (entry.epochMsec > until_msecs) {
				completed = true;
				break;
			}
			m_mergedLog.append(entry);
			if ((int)m_mergedLog.entries().size() >= m_mergedLog.inputFilterRecordCountLimit()) {
				completed = true;
				break;
			}
		}
		if (completed) {
			break;
		}
	}
	if (logs.count() == 0) {
		logs = log_dir.findFiles(QDateTime(), QDateTime());
		if (logs.count()) {
			ShvLogFileReader log_reader(logs.last().toStdString());
			header = log_reader.logHeader();
		}
	}
	if (!completed && log_dir.exists(log_dir.dirtyLogName())) {
		ShvJournalFileReader log_reader(log_dir.dirtyLogPath().toStdString(), &header);
		while (log_reader.next()) {
			const ShvJournalEntry &entry = log_reader.entry();
			if (entry.epochMsec > until_msecs) {
				break;
			}
			m_mergedLog.append(entry);
			if ((int)m_mergedLog.entries().size() >= m_mergedLog.inputFilterRecordCountLimit()) {
				break;
			}
		}
	}
	return m_mergedLog;
}
