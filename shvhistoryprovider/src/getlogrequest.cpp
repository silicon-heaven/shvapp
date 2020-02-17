#include "application.h"
#include "logdir.h"
#include "devicemonitor.h"
#include "getlogrequest.h"

#include <QDateTime>

#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvlogfilereader.h>
#include <shv/coreqt/exception.h>
#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/rpc.h>

namespace cp = shv::chainpack;
using namespace shv::core::utils;

GetLogRequest::GetLogRequest(const QString &shv_path, const shv::core::utils::ShvGetLogParams &log_params)
	: m_shvPath(shv_path)
	, m_logParams(log_params)
{
}

shv::core::utils::ShvMemoryJournal &GetLogRequest::getLog()
{
	QDateTime since = rpcvalue_cast<QDateTime>(m_logParams.since);
	QDateTime until = rpcvalue_cast<QDateTime>(m_logParams.until);

	if (!Application::instance()->deviceMonitor()->monitoredDevices().contains(m_shvPath)) {
		SHV_EXCEPTION("Invalid shv_path");
	}
	LogDir log_dir(m_shvPath);
	QStringList logs = log_dir.findFiles(since, until);
	qint64 until_msecs = until.isNull() ? -1 : until.toMSecsSinceEpoch();

	bool completed = false;

	ShvLogHeader header;
	for (const QString &log : logs) {
		shv::core::utils::ShvGetLogParams params;
		params.maxRecordCount = std::numeric_limits<int>().max();
		ShvMemoryJournal partial_journal(params);

		ShvLogFileReader log_reader(log.toStdString());
		header = log_reader.logHeader();
		while (log_reader.next()) {
			const ShvJournalEntry &entry = log_reader.entry();
			if (until_msecs != -1 && entry.epochMsec > until_msecs) {
				completed = true;
				break;
			}
			m_result.append(entry);
			if ((int)m_result.entries().size() >= m_logParams.maxRecordCount) {
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
			if (until_msecs != -1 && entry.epochMsec > until_msecs) {
				break;
			}
			m_result.append(entry);
			if ((int)m_result.entries().size() >= m_logParams.maxRecordCount) {
				break;
			}
		}
	}
	return m_result;
}
