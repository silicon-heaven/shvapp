#pragma once

#include <shv/coreqt/log.h>

inline NecroLog &operator<<(NecroLog log, const QStringView &s)
{
	return log << s.toString().toStdString();
}
