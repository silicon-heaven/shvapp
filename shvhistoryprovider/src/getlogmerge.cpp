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
	if (log_params.since.asString() == ShvGetLogParams::SINCE_NOW && m_shvPaths.count() > 1) {
		SHV_EXCEPTION("Since now is valid only with single device");
	}
}

shv::chainpack::RpcValue GetLogMerge::getLog()
{
	bool since_now = (m_logParams.since.asString() == ShvGetLogParams::SINCE_NOW);
	QDateTime since = since_now ? QDateTime::currentDateTimeUtc() : rpcvalue_cast<QDateTime>(m_logParams.since);
	QDateTime until = rpcvalue_cast<QDateTime>(m_logParams.until);

	int64_t first_record_since = 0LL;

	struct ReaderInfo {
		bool used = false;
		bool exhausted = false;
	};

	ShvGetLogParams first_step_params;
	ShvGetLogParams final_params;
	if (!m_logParams.pathPattern.empty()) {
		first_step_params.pathPattern = m_logParams.pathPattern;
		first_step_params.pathPatternType = m_logParams.pathPatternType;
	}
	if (since.isValid()) {
		if (m_logParams.withSnapshot) {
			final_params.since = m_logParams.since;
		}
		else {
			first_step_params.since = m_logParams.since;
		}
	}
	ShvLogFilter first_step_filter(first_step_params);
	ShvLogFilter final_filter(final_params);

	QVector<LogDirReader*> readers;
	QVector<ReaderInfo> reader_infos;
	for (const QString &shv_path : m_shvPaths) {
		int prefix_length = shv_path.length() - m_shvPath.length();
		LogDirReader *reader = new LogDirReader(shv_path, prefix_length, since, until, m_logParams.withSnapshot);
		if (reader->next()) {
			readers << reader;
			reader_infos << ReaderInfo();
		}
		else {
			delete reader;
		}
	}

	int64_t until_msecs = until.isNull() ? std::numeric_limits<int64_t>::max() : until.toMSecsSinceEpoch();
	int64_t last_record_ts = 0LL;
	int record_count = 0;
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
		if (first_step_filter.match(entry)) {
			if (!first_record_since) {
				first_record_since = entry.epochMsec;
			}
			m_mergedLog.append(entry);
			last_record_ts = entry.epochMsec;
			if (!since_now) {
				if (final_filter.match(entry)) {
					++record_count;
					if (record_count > m_logParams.recordCountLimit) {
						break;
					}
				}
			}
		}
		reader_infos[oldest_index].used = true;
		if (!reader->next()) {
			reader_infos[oldest_index].exhausted = true;
			--usable_readers;
		}
	}
	cp::RpcValue::Map dirty_log_info;
	for (int i = 0; i < readers.count(); ++i) {
		if (reader_infos[i].used) {
			m_mergedLog.setTypeInfo(readers[i]->typeInfo(), readers[i]->pathPrefix());
			if (readers[i]->dirtyLogBegin()) {
				dirty_log_info[readers[i]->pathPrefix()] = cp::RpcValue::Map{{ "startTS", cp::RpcValue::DateTime::fromMSecsSinceEpoch(readers[i]->dirtyLogBegin()) }};
			}
		}
	}
	qDeleteAll(readers);

	ShvGetLogParams params = m_logParams;
	if (since_now || params.since.toDateTime().msecsSinceEpoch() > last_record_ts) {
		if (last_record_ts) {
			params.since = cp::RpcValue::DateTime::fromMSecsSinceEpoch(last_record_ts);
		}
		else {
			params.since = cp::RpcValue::DateTime::now();
		}
	}
	params.pathPattern = std::string();
	cp::RpcValue res = m_mergedLog.getLog(params);
	if (since_now) {
		if (last_record_ts) {
			res.setMetaValue("since", cp::RpcValue::DateTime::fromMSecsSinceEpoch(last_record_ts));
		}
		else {
			res.setMetaValue("since", cp::RpcValue::DateTime::now());
		}
	}
	else if (!since.isNull() && first_record_since && first_record_since > since.toMSecsSinceEpoch()) {
		res.setMetaValue("since", cp::RpcValue::DateTime::fromMSecsSinceEpoch(first_record_since));
	}
	res.setMetaValue("dirtyLog", dirty_log_info);
	return res;
}
