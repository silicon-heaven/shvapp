#pragma once

#include "shvlog.h"

//#define SHV_NO_DEBUG
#ifndef SHV_NO_DEBUG_LOG
	#ifdef QT_NO_DEBUG
		#define SHV_NO_DEBUG
	#endif
#endif

#ifdef SHV_NO_DEBUG_LOG
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

#ifdef SHV_NO_DEBUG_LOG
#define shvLogFuncFrame() while(0) shvDebug()
#else
#define shvLogFuncFrame() shv::core::ShvLog __shv_func_frame_exit_logger__ = shv::core::ShvLog::isMatchingLogFilter(shv::core::ShvLog::Level::Debug, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__})? \
	shv::core::ShvLog(shv::core::ShvLog::Level::Debug, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__}) << "     EXIT FN" << __FUNCTION__: \
	shv::core::ShvLog(shv::core::ShvLog::Level::Invalid, shv::core::ShvLog::LogContext{__FILE__, __LINE__, __FUNCTION__}); \
	shvDebug() << ">>>> ENTER FN" << __FUNCTION__
#endif
