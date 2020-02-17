#ifndef GETLOGREQUEST_H
#define GETLOGREQUEST_H

#include "abstractrequest.h"

#include <shv/core/utils/shvmemoryjournal.h>

class GetLogRequest
{
public:
	explicit GetLogRequest(const QString &shv_path, const shv::core::utils::ShvGetLogParams &log_params);

	shv::core::utils::ShvMemoryJournal &getLog();

private:
	QString m_shvPath;
	shv::core::utils::ShvGetLogParams m_logParams;
	shv::core::utils::ShvMemoryJournal m_result;
};

#endif // GETLOGREQUEST_H
