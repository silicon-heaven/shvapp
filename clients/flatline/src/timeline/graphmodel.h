#pragma once

#include "graph.h"

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

	using timemsec_t = Graph::timemsec_t;
	struct Sample {
		timemsec_t time = 0;
		QVariant value;
		//timemsec_t origtime = 0; // sometimes is needed to show samples in transformed time scale (hide empty areas without samples)

		Sample() {}
		Sample(timemsec_t t, const QVariant &v) : time(t), value(v) {}
		Sample(timemsec_t t, QVariant &&v) : time(t), value(std::move(v)) {}

		//inline timemsec_t displayTime() const {return origtime != 0? origtime: time;}
	};
public:
	explicit GraphModel(QObject *parent = nullptr);
public: // API
	virtual int channelCount() const { return qMin(m_channelsData.count(), m_samples.count()); }
	virtual QVariant channelData(int channel, ChannelDataRole::Enum role) const;
	virtual void setChannelData(int channel, const QVariant &v, ChannelDataRole::Enum role);

	virtual int count(int channel) const;
	virtual Sample value(int channel, int ix) const;
	/// sometimes is needed to show samples in transformed time scale (hide empty areas without samples)
	/// displayValue() returns original time and value
	virtual Sample displayValue(int channel, int ix) const { return value(channel, ix); }

	virtual int lessOrEqualIndex(int channel, timemsec_t time) const;

	virtual void beginAppendValues();
	virtual void endAppendValues();
	virtual void appendValue(int channel, Sample &&value);
	Q_SIGNAL void valuesAppended(timemsec_t since, timemsec_t until);

	void appendChannel();
public:
	//QPair<double, double> range(int channel);
protected:
	using ChannelSamples = QVector<Sample>;
	QVector<ChannelSamples> m_samples;
	using ChannelData = QMap<int, QVariant>;
	QVector<ChannelData> m_channelsData;
	timemsec_t m_appendSince, m_appendUntil;
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
