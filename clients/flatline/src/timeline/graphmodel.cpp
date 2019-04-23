#include "graphmodel.h"

#include <shv/coreqt/log.h>

namespace timeline {

GraphModel::GraphModel(QObject *parent)
	: Super(parent)
{

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

GraphModel::Sample GraphModel::value(int channel, int ix) const
{
	if(channel < 0 || channel > channelCount())
		return Sample();
	return m_samples.at(channel).value(ix);
}

template<class ForwardIt, class T>
ForwardIt lower_bound(ForwardIt first, ForwardIt last, const T& value)
{
	ForwardIt it;
	typename std::iterator_traits<ForwardIt>::difference_type count, step;
	count = std::distance(first, last);

	while (count > 0) {
		it = first;
		step = count / 2;
		std::advance(it, step);
		if (*it < value) {
			first = ++it;
			count -= step + 1;
		}
		else
			count = step;
	}
	return first;
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
		if (value(channel, pivot).time <= time) {
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

void GraphModel::appendValue(int channel, GraphModel::Sample &&value)
{
	if(channel < 0 || channel > channelCount()) {
		shvError() << "Invalid channel index:" << channel;
		return;
	}
	if(value.time <= 0) {
		shvWarning() << "ignoring value with timestamp <= 0, timestamp:" << value.time;
		return;
	}
	ChannelSamples &dat = m_samples[channel];
	if(!dat.isEmpty() && dat.last().time > value.time) {
		shvWarning() << "ignoring value with lower timestamp than last value:" << value.time;
		return;
	}
	m_appendSince = qMin(value.time, m_appendSince);
	m_appendUntil = qMax(value.time, m_appendUntil);
	dat.push_back(std::move(value));
}

void GraphModel::appendChannel()
{
	m_channelsData.append(ChannelData());
}

} // namespace timeline
