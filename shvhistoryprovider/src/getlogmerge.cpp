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
	if (log_params.isSinceLast() && m_shvPaths.count() > 1) {
		SHV_EXCEPTION("Since last is valid only with single device");
	}
}

shv::chainpack::RpcValue GetLogMerge::getLog()
{
	bool since_last = m_logParams.isSinceLast();
	QDateTime since = since_last ? QDateTime::currentDateTimeUtc() : rpcvalue_cast<QDateTime>(m_logParams.since);
	QDateTime until = rpcvalue_cast<QDateTime>(m_logParams.until);

	int64_t first_record_since = 0LL;

	struct ReaderInfo {
		QString shvPath;
		bool used = false;
		bool exhausted = false;
	};

	ShvGetLogParams log_params;
	if (!m_logParams.pathPattern.empty()) {
		log_params.pathPattern = m_logParams.pathPattern;
		log_params.pathPatternType = m_logParams.pathPatternType;
	}
	if (since.isValid() && !m_logParams.withSnapshot) {
		log_params.since = m_logParams.since;
	}
	int64_t param_since = m_logParams.since.isDateTime() ? m_logParams.since.toDateTime().msecsSinceEpoch() : 0;

	ShvLogFilter first_step_filter(log_params);

	QVector<LogDirReader*> readers;
	QVector<ReaderInfo> reader_infos;
	for (const QString &shv_path : m_shvPaths) {
		const SitesHPDevice *site_item = qobject_cast<const SitesHPDevice *>(Application::instance()->deviceMonitor()->sites()->itemByShvPath(shv_path));
		int prefix_length = shv_path.length() - m_shvPath.length();
		LogDirReader *reader = new LogDirReader(shv_path, site_item->isPushLog(), prefix_length, since, until, m_logParams.withSnapshot);
		if (reader->next()) {
			readers << reader;
			reader_infos << ReaderInfo();
			reader_infos.last().shvPath = shv_path;
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
			if (!since_last && entry.epochMsec >= param_since) {
				if (++record_count > m_logParams.recordCountLimit) {
					break;
				}
			}
		}
		reader_infos[oldest_index].used = true;
		if (!reader->next()) {
			reader_infos[oldest_index].exhausted = true;
			--usable_readers;
		}
	}
	cp::RpcValue::List dirty_log_info;
	for (int i = 0; i < readers.count(); ++i) {
		if (reader_infos[i].used) {
			m_mergedLog.setTypeInfo(readers[i]->typeInfo(), readers[i]->pathPrefix());
			if (readers[i]->dirtyLogBegin()) {
				cp::RpcValue::Map m {
					{ "startTS", cp::RpcValue::DateTime::fromMSecsSinceEpoch(readers[i]->dirtyLogBegin())},
					{ "path", readers[i]->pathPrefix()},
				};
				dirty_log_info.push_back(m);
			}
		}
	}
	qDeleteAll(readers);

	ShvGetLogParams params = m_logParams;
	if (since_last) { // || params.since.toDateTime().msecsSinceEpoch() > last_record_ts) {
		if (last_record_ts) {
			params.since = cp::RpcValue::DateTime::fromMSecsSinceEpoch(last_record_ts);
		}
		else {
			params.since = cp::RpcValue::DateTime::now();
		}
	}
	params.pathPattern = std::string();
	cp::RpcValue res = m_mergedLog.getLog(params);
	if (since_last) {
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

	if (reader_infos.count() == 1 && reader_infos[0].exhausted) {
		const QString &shv_path = reader_infos[0].shvPath;
		LogDir log_dir(shv_path);
		if (!log_dir.existsDirtyLog()) {
			QStringList all_logs = log_dir.findFiles(QDateTime(), QDateTime());

			if (all_logs.count()) {
				ShvLogFileReader log_reader(all_logs.last().toStdString());
				const ShvLogHeader &header = log_reader.logHeader();
				if (header.until().toDateTime().msecsSinceEpoch() < until_msecs) {
					res.setMetaValue("until", header.until());
				}
			}
		}
	}

	res.setMetaValue("dirtyLog", dirty_log_info);
	return res;
}
