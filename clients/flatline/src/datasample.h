#ifndef DATASAMPLE_H
#define DATASAMPLE_H

#include <cstddef>

class QColor;

struct DataSample
{
	enum {Voltage = 0, Current, Power, CHANNEL_CNT};
	unsigned time = 0;
	int values[CHANNEL_CNT];

	static QColor channelColor(unsigned channel_ix);
};

#endif // DATASAMPLE_H
