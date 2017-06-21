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
/*
#if (QT_VERSION < QT_VERSION_CHECK(5, 5, 0))
#define qfCInfo(category) for(bool en = qf::core::LogDevice::isMatchingAnyDeviceLogFilter(qf::core::Log::Level::Info, __FILE__, category); en; en = false) \
	QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO, category).warning
#else
#define qfCInfo(category) for(bool en = qf::core::LogDevice::isMatchingAnyDeviceLogFilter(qf::core::Log::Level::Info, __FILE__, category); en; en = false) \
	QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO, category).info
#endif

#define qfCWarning(category) for(bool en = qf::core::LogDevice::isMatchingAnyDeviceLogFilter(qf::core::Log::Level::Warning, __FILE__, category); en; en = false) \
	QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO, category).warning
#define qfCError(category) for(bool en = qf::core::LogDevice::isMatchingAnyDeviceLogFilter(qf::core::Log::Level::Error, __FILE__, category); en; en = false) \
	QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO, category).critical
*/
#define shvDebug() shvCDebug("")
/*
#define qfInfo qfCInfo("default")
#define qfWarning qfCWarning("default")
#define qfError qfCError("default")

#ifdef NO_QF_DEBUG
#define qfLogFuncFrame() while(0) qDebug()
#else
#define qfLogFuncFrame() QDebug __qf_func_frame_exit_logger__ = qf::core::LogDevice::isMatchingAnyDeviceLogFilter(qf::core::Log::Level::Debug, __FILE__, "")? QMessageLogger(__FILE__, __LINE__, Q_FUNC_INFO, "").debug() << "     EXIT FN" << Q_FUNC_INFO: QMessageLogger().debug(); \
	qfDebug() << ">>>> ENTER FN" << Q_FUNC_INFO
#endif
*/
