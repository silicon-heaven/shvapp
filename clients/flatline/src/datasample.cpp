#include "datasample.h"

#include <QColor>

QColor DataSample::channelColor(unsigned channel_ix)
{
	static QColor colors[DataSample::CHANNEL_CNT + 1] = {Qt::magenta, Qt::cyan, Qt::yellow, Qt::red};
	if(channel_ix < DataSample::CHANNEL_CNT)
		return colors[channel_ix];
	return colors[DataSample::CHANNEL_CNT];
}
