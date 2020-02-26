#include "application.h"
#include "devicemonitor.h"
#include "getlogmerge.h"
#include "logdir.h"
#include "logdirreader.h"
#include "siteitem.h"

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

shv::chainpack::RpcValue GetLogMerge::getLog()
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

	return m_mergedLog.getLog(m_logParams);
}
