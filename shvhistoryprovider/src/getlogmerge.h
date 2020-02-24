#ifndef GETLOGREQUEST_H
#define GETLOGREQUEST_H

#include "asyncrequest.h"

#include <shv/core/utils/shvmemoryjournal.h>

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
};

#endif // GETLOGREQUEST_H
