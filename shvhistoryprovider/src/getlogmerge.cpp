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

	struct ReaderInfo {
		bool used = false;
		bool exhausted = false;
	};

	QVector<LogDirReader*> readers;
	QVector<ReaderInfo> reader_infos;
	for (const QString &shv_path : m_shvPaths) {
		int prefix_length = shv_path.length() - m_shvPath.length();
		LogDirReader *reader = new LogDirReader(shv_path, prefix_length, since, until);
		if (reader->next()) {
			readers << reader;
			reader_infos << ReaderInfo();
		}
		else {
			delete reader;
		}
	}

	int64_t until_msecs = until.isNull() ? std::numeric_limits<int64_t>::max() : until.toMSecsSinceEpoch();

	int usable_readers = readers.count();
	while (usable_readers) {
		int64_t oldest = std::numeric_limits<int64_t>::max();
		int oldest_index = -1;
		for (int i = 0; i < readers.count(); ++i) {
			if (!reader_infos[i].exhausted && readers[i]->entry().epochMsec < oldest) {
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
		if ((int)m_mergedLog.size() >= m_mergedLog.inputFilterRecordCountLimit()) {
			break;
		}
		reader_infos[oldest_index].used = true;
		if (!reader->next()) {
			reader_infos[oldest_index].exhausted = true;
			--usable_readers;
		}
	}
	for (int i = 0; i < readers.count(); ++i) {
		if (reader_infos[i].used) {
			m_mergedLog.setTypeInfo(readers[i]->pathPrefix(), readers[i]->typeInfo());
		}
	}
	qDeleteAll(readers);

	return m_mergedLog.getLog(m_logParams);;
}
