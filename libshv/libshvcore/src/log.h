#pragma once

#include "shvlog.h"

//#define NO_QF_DEBUG

//#define qfFatal if(qCritical() << qf::core::Log::stackTrace(), true) qFatal

#ifdef NO_QF_DEBUG
#define shvCDebug(category) while(false) shv::core::ShvLog(shv::core::ShvLog::Level::Debug, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category})
#else
#define shvCDebug(category) for(bool en = shv::core::ShvLog::isMatchingLogFilter(shv::core::ShvLog::Level::Debug, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category}); en; en = false) \
	shv::core::ShvLog(shv::core::ShvLog::Level::Debug, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category})
#endif
#define shvCInfo(category) for(bool en = shv::core::ShvLog::isMatchingLogFilter(shv::core::ShvLog::Level::Info, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category}); en; en = false) \
	shv::core::ShvLog(shv::core::ShvLog::Level::Info, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category})
#define shvCWarning(category) for(bool en = shv::core::ShvLog::isMatchingLogFilter(shv::core::ShvLog::Level::Warning, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category}); en; en = false) \
	shv::core::ShvLog(shv::core::ShvLog::Level::Warning, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category})
#define shvCError(category) for(bool en = shv::core::ShvLog::isMatchingLogFilter(shv::core::ShvLog::Level::Error, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category}); en; en = false) \
	shv::core::ShvLog(shv::core::ShvLog::Level::Error, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__, category})

#define shvDebug() shvCDebug("")
#define shvInfo() shvCInfo("")
#define shvWarning() shvCWarning("")
#define shvError() shvCError("")
