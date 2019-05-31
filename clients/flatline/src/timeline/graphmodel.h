#pragma once

#include "graph.h"
#include "sample.h"

#include <QObject>
#include <QVariant>
#include <QVector>

namespace timeline {

class GraphModel : public QObject
{
	Q_OBJECT

	using Super = QObject;
public:
	struct ChannelDataRole {enum Enum {ShvPath, Name, UserData = 64};};
	//struct ValueRole {enum Enum {Display, Label, UserData = 64};};

	SHV_FIELD_BOOL_IMPL2(a, A, utoCreateChannels, true)

public:
	explicit GraphModel(QObject *parent = nullptr);

	XRange xRange() const;
	XRange xRange(int channel_ix) const;
	YRange yRange(int channel_ix) const;
	void clear();
	void appendChannel() {appendChannel(std::string(), std::string());}
	void appendChannel(const std::string &shv_path, const std::string &name);
	virtual int guessMetaType(int channel_ix);
public: // API
	virtual int channelCount() const { return qMin(m_channelsData.count(), m_samples.count()); }
	virtual QVariant channelData(int channel, ChannelDataRole::Enum role) const;
	virtual void setChannelData(int channel, const QVariant &v, ChannelDataRole::Enum role);

	virtual int count(int channel) const;
	/// without bounds check
	virtual Sample sampleAt(int channel, int ix) const;
	/// returns Sample() if out of bounds
	Sample sampleValue(int channel, int ix) const;
	/// sometimes is needed to show samples in transformed time scale (hide empty areas without samples)
	/// displaySampleValue() returns original time and value
	virtual Sample displaySampleValue(int channel, int ix) const { return sampleValue(channel, ix); }

	virtual int lessOrEqualIndex(int channel, timemsec_t time) const;

	virtual void beginAppendValues();
	virtual void endAppendValues();
	virtual void appendValue(int channel, Sample &&sample);
	void appendValueShvPath(const std::string &shv_path, Sample &&sample);

	int pathToChannel(const std::string &path) const;

	Q_SIGNAL void xRangeChanged(XRange range);
	Q_SIGNAL void channelCountChanged(int cnt);
public:
	static double valueToDouble(const QVariant v);
protected:
	using ChannelSamples = QVector<Sample>;
	QVector<ChannelSamples> m_samples;
	using ChannelData = QMap<int, QVariant>;
	QVector<ChannelData> m_channelsData;
	XRange m_begginAppendXRange;

	mutable std::map<std::string, int> m_pathToChannelCache;
};
/*
class GraphModel : public AbstractGraphModel
{
	Q_OBJECT

	using Super = AbstractGraphModel;

	explicit GraphModel(QObject *parent = nullptr);

	int channelCount() override;
	QVariant channelData(SerieDataRole::Enum role) override;
	int count(int channel) override;
	Data data(int channel, int ix) override;

	int lessOrEqualIndex(int channel, timemsec_t time) override;

	void beginAppendData() override;
	void endAppendData() override;
	void appendData(int channel, Data &&data) override;
private:
	using Serie = QVector<Data>;
	QVector<Serie> m_data;
};
*/
} // namespace timeline
