#ifndef GETLOGMERGE_H
#define GETLOGMERGE_H

#include <shv/core/utils/shvmemoryjournal.h>
#include <shv/core/utils/shvlogfilter.h>

#include <QString>
#include <QStringList>

class GetLogMerge
{
public:
	explicit GetLogMerge(const QString &shv_path, const shv::core::utils::ShvGetLogParams &log_params);

	shv::chainpack::RpcValue getLog();

private:
	QString m_shvPath;
	QStringList m_shvPaths;
	shv::core::utils::ShvGetLogParams m_logParams;
	shv::core::utils::ShvMemoryJournal m_mergedLog;
	shv::core::utils::ShvLogFilter m_logFilter;
};

#endif // GETLOGMERGE_H
