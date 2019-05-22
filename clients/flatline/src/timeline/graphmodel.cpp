#include "graphmodel.h"

#include <shv/coreqt/log.h>

namespace timeline {

GraphModel::GraphModel(QObject *parent)
	: Super(parent)
{

}

void GraphModel::clear()
{
	m_samples.clear();
	m_channelsData.clear();
}

QVariant GraphModel::channelData(int channel, GraphModel::ChannelDataRole::Enum role) const
{
	if(channel < 0 || channel > channelCount()) {
		return QVariant();
	}
	const ChannelData &chp = m_channelsData[channel];
	return chp.value(role);
}

void GraphModel::setChannelData(int channel, const QVariant &v, GraphModel::ChannelDataRole::Enum role)
{
	if(channel < 0 || channel > channelCount()) {
		shvError() << "Invalid channel index:" << channel;
		return;
	}
	ChannelData &chp = m_channelsData[channel];
	chp[role] = v;
}

int GraphModel::count(int channel) const
{
	if(channel < 0 || channel > channelCount())
		return 0;
	return m_samples.at(channel).count();
}

Sample GraphModel::sampleAt(int channel, int ix) const
{
	return m_samples.at(channel).at(ix);
}

Sample GraphModel::sampleValue(int channel, int ix) const
{
	if(channel < 0 || channel >= channelCount())
		return Sample();
	if(ix < 0 || ix >= m_samples[channel].count())
		return Sample();
	return sampleAt(channel, ix);
}

XRange GraphModel::xRange() const
{
	XRange ret;
	for (int i = 0; i < channelCount(); ++i) {
		if(i == 0) {
			ret = xRange(i);
		}
		else {
			XRange r = xRange(i);
			ret.min = qMin(ret.min, r.min);
			ret.max = qMax(ret.max, r.max);
		}
	}
	return ret;
}

XRange GraphModel::xRange(int channel_ix) const
{
	XRange ret;
	if(count(channel_ix) > 0) {
		ret.min = sampleAt(channel_ix, 0).time;
		ret.max = sampleAt(channel_ix, count(channel_ix) - 1).time;
	}
	return ret;
}

YRange GraphModel::yRange(int channel_ix) const
{
	YRange ret;
	for (int i = 0; i < count(channel_ix); ++i) {
		QVariant v = sampleAt(channel_ix, i).value;
		double d = valueToDouble(v);
		ret.min = qMin(ret.min, d);
		ret.max = qMax(ret.max, d);
	}
	return ret;
}

double GraphModel::valueToDouble(const QVariant v)
{
	switch (v.type()) {
	case QVariant::Invalid:
		return 0;
	case QVariant::Double:
		return v.toDouble();
	case QVariant::LongLong:
	case QVariant::ULongLong:
	case QVariant::UInt:
	case QVariant::Int:
		return v.toLongLong();
	case QVariant::Bool:
		return v.toBool()? 1: 0;
	case QVariant::String:
		return v.toString().isEmpty()? 0: 1;
	default:
		shvWarning() << "cannot convert variant:" << v.typeName() << "to double";
		return 0;
	}
}

int GraphModel::lessOrEqualIndex(int channel, timemsec_t time) const
{
	if(channel < 0 || channel > channelCount())
		return -1;

	int first = 0;
	int cnt = count(channel);
	bool found = false;
	while (cnt > 0) {
		int step = cnt / 2;
		int pivot = first + step;
		if (sampleAt(channel, pivot).time <= time) {
			first = pivot;
			if(step)
				cnt -= step;
			else
				cnt = 0;
			found = true;
		}
		else {
			cnt = step;
			found = false;
		}
	};
	int ret = found? first: -1;
	//shvInfo() << time << "-->" << ret;
	return ret;
}

void GraphModel::beginAppendValues()
{
	m_begginAppendXRange = xRange();
}

void GraphModel::endAppendValues()
{
	XRange xr = xRange();
	if(xr.max > m_begginAppendXRange.max)
		emit xRangeChanged(xr);
	m_begginAppendXRange = XRange();
}

void GraphModel::appendValue(int channel, Sample &&sample)
{
	if(channel < 0 || channel > channelCount()) {
		shvError() << "Invalid channel index:" << channel;
		return;
	}
	if(sample.time <= 0) {
		shvWarning() << "ignoring value with timestamp <= 0, timestamp:" << sample.time;
		return;
	}
	ChannelSamples &dat = m_samples[channel];
	if(!dat.isEmpty() && dat.last().time > sample.time) {
		shvWarning() << channelData(channel, ChannelDataRole::ShvPath).toString() << "channel:" << channel
					 << "ignoring value with lower timestamp than last value:" << dat.last().time << "val:" << sample.time;
		return;
	}
	//m_appendSince = qMin(sampleAt.time, m_appendSince);
	//m_appendUntil = qMax(sampleAt.time, m_appendUntil);
	dat.push_back(std::move(sample));
}

void GraphModel::appendChannel()
{
	m_channelsData.append(ChannelData());
	m_samples.append(ChannelSamples());
}

int GraphModel::guessMetaType(int channel_ix)
{
	Sample s = sampleValue(channel_ix, 0);
	return s.value.userType();
}

} // namespace timeline
