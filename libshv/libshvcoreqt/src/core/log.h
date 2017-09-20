#pragma once

#include <shv/core/log.h>

#include <QString>

inline shv::core::ShvLog &operator<<(shv::core::ShvLog &log, const QString &s) { return log.operator <<(s.toUtf8().constData()); }
