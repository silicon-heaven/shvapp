#pragma once

#include <QDateTime>
#include <QObject>

class DateTimeInterval
{
public:
	QDateTime since;
	QDateTime until;
};

Q_DECLARE_METATYPE(DateTimeInterval)
