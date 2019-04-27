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

GraphModel::Sample GraphModel::sampleAt(int channel, int ix) const
{
	return m_samples.at(channel).at(ix);
}

GraphModel::Sample GraphModel::sampleValue(int channel, int ix) const
{
	if(channel < 0 || channel >= channelCount())
		return Sample();
	if(ix < 0 || ix >= m_samples[channel].count())
		return Sample();
	return sampleAt(channel, ix);
}

QPair<GraphModel::timemsec_t, GraphModel::timemsec_t> GraphModel::xRange() const
{
	QPair<GraphModel::timemsec_t, GraphModel::timemsec_t> ret{0, 0};
	for (int i = 0; i < channelCount(); ++i) {
		if(i == 0) {
			ret = xRange(i);
		}
		else {
			QPair<timemsec_t, timemsec_t> r = xRange(i);
			ret.first = qMin(ret.first, r.first);
			ret.second = qMax(ret.second, r.second);
		}
	}
	return ret;
}

QPair<GraphModel::timemsec_t, GraphModel::timemsec_t> GraphModel::xRange(int channel_ix) const
{
	QPair<GraphModel::timemsec_t, GraphModel::timemsec_t> ret{0, 0};
	if(count(channel_ix) > 0) {
		ret.first = sampleAt(channel_ix, 0).time;
		ret.second = sampleAt(channel_ix, count(channel_ix) - 1).time;
	}
	return ret;
}

QPair<double, double> GraphModel::yRange(int channel_ix) const
{
	//QPair<double, double> ret{std::numeric_limits<double>::max(), std::numeric_limits<double>::min()};
	QPair<double, double> ret{0, 0};
	for (int i = 0; i < count(channel_ix); ++i) {
		QVariant v = sampleAt(channel_ix, i).value;
		double d = valueToDouble(v);
		if(d < ret.first)
			ret.first = d;
		if(d > ret.second)
			ret.second = d;
	}
	return ret;
}

double GraphModel::valueToDouble(const QVariant v)
{
	switch (v.type()) {
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

int GraphModel::lessOrEqualIndex(int channel, GraphModel::timemsec_t time) const
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
			cnt = cnt - step;
			found = true;
		}
		else {
			cnt = step;
			found = false;
		}
	}
	return found? first: -1;
}

void GraphModel::beginAppendValues()
{
	m_appendSince = m_appendUntil = 0;
}

void GraphModel::endAppendValues()
{
	if(m_appendSince > 0)
		emit valuesAppended(m_appendSince, m_appendUntil);
}

void GraphModel::appendValue(int channel, GraphModel::Sample &&sampleAt)
{
	if(channel < 0 || channel > channelCount()) {
		shvError() << "Invalid channel index:" << channel;
		return;
	}
	if(sampleAt.time <= 0) {
		shvWarning() << "ignoring value with timestamp <= 0, timestamp:" << sampleAt.time;
		return;
	}
	ChannelSamples &dat = m_samples[channel];
	if(!dat.isEmpty() && dat.last().time > sampleAt.time) {
		shvWarning() << "ignoring value with lower timestamp than last value:" << dat.last().time << "val:" << sampleAt.time;
		return;
	}
	m_appendSince = qMin(sampleAt.time, m_appendSince);
	m_appendUntil = qMax(sampleAt.time, m_appendUntil);
	dat.push_back(std::move(sampleAt));
}

void GraphModel::appendChannel()
{
	m_channelsData.append(ChannelData());
	m_samples.append(ChannelSamples());
}

} // namespace timeline
