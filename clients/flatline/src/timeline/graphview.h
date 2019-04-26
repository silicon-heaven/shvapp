#pragma once

#include "graph.h"

#include <shv/coreqt/utils.h>

#include <QPointer>
#include <QScrollArea>
#include <QVector>
#include <QVariantMap>

namespace timeline {

class GraphModel;

class GraphView : public QScrollArea
{
	Q_OBJECT

	using Super = QScrollArea;
public:
	using timemsec_t = Graph::timemsec_t;
	class GraphStyle : public QVariantMap
	{
		SHV_VARIANTMAP_FIELD2(double, l, setL, eftMargin, 0.3) // units
		SHV_VARIANTMAP_FIELD2(double, r, setR, ightMargin, 0.1) // units
		SHV_VARIANTMAP_FIELD2(double, t, setT, opMargin, 0.1) // units
		SHV_VARIANTMAP_FIELD2(double, b, setb, ottomMargin, 0.1) // units
		SHV_VARIANTMAP_FIELD2(double, c, setC, hannelSpacing, 1) // units
		SHV_VARIANTMAP_FIELD2(double, x, setX, AxisHeight, 1) // units
		SHV_VARIANTMAP_FIELD2(double, y, setY, AxisWidth, 2) // units
		SHV_VARIANTMAP_FIELD2(double, n, setN, avigationBarHeight, 2) // units
		SHV_VARIANTMAP_FIELD2(double, v, setV, erticalHeaderWidth, 6) // units
		SHV_VARIANTMAP_FIELD2(bool, s, setS, eparateChannels, true)

		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olor, QColor(Qt::yellow))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorBackground, QColor(Qt::darkGray))
	};
	class ChannelStyle : public QVariantMap
	{
		SHV_VARIANTMAP_FIELD2(double, h, setH, eightMin, 2) // units
		SHV_VARIANTMAP_FIELD2(double, h, setH, eightMax, 2) // units
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olor, QColor(Qt::magenta))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorGrid, QColor(Qt::green))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorBackground, QColor(Qt::black))

	public:
		ChannelStyle() : QVariantMap() {}
		ChannelStyle(const QVariantMap &o) : QVariantMap(o) {}

		static constexpr double HEIGHT_HUGE = 10e3;

		double heightRange() const
		{
			if(!heightMax_isset())
				return HEIGHT_HUGE;
			double ret = heightMax() - heightMin();
			return ret < 0? 0: ret;
		}
	};

	struct Options
	{
		int unitToPx = 0;
		GraphStyle style;
		ChannelStyle defaultChannelStyle;
	};
	struct Channel
	{
		int metaTypeId = 0;

		double yRangeMin = 0;
		double yRangeMax = 0;
		double yRange() const { return yRangeMax - yRangeMin; }

		ChannelStyle style;

		struct
		{
			QRect graphRect;
			QRect verticalHeaderRect;
			QRect yAxisRect;
			double yRangeDisplMin = 0;
			double yRangeDisplMax = 0;
		} state;
	};
	struct State
	{
		timemsec_t xRangeMin = 0;
		timemsec_t xRangeMax = 0;
		double xRange() const { return xRangeMax - xRangeMin; }
		timemsec_t xRangeDisplMin = 0;
		timemsec_t xRangeDisplMax = 0;

		QRect navigationBarRect;
		QRect xAxisRect;
	};

public:
	GraphView(QWidget *parent = nullptr);

	void setModel(GraphModel *model);
	GraphModel *model();
	void createChannelsFromModel();
protected:
	QVariantMap mergeMaps(const QVariantMap &base, const QVariantMap &overlay) const;
	int u2px(double unit) { return static_cast<int>(m_options.unitToPx * unit); }
	void makeLayout();

	virtual void draw(QPainter *painter);

	virtual void drawBackground(QPainter *painter);
	virtual void drawNavigationBar(QPainter *painter);
	virtual void drawXAxis(QPainter *painter);

	struct DrawChannelOptions
	{
		QRect rect;
		ChannelStyle channelStyle;
		GraphStyle graphStyle;
	};
	//virtual void drawGraph(int channel);
	virtual void drawVerticalHeader(QPainter *painter, int channel, const DrawChannelOptions &options);
	virtual void drawBackground(QPainter *painter, int channel, const DrawChannelOptions &options);
	virtual void drawGrid(QPainter *painter, int channel, const DrawChannelOptions &options);
	virtual void drawYAxis(QPainter *painter, int channel, const DrawChannelOptions &options);
	virtual void drawSamples(QPainter *painter, int channel, const DrawChannelOptions &options);

	// QWidget interface
protected:
	void paintEvent(QPaintEvent *event) override;
protected:
	QVector<Channel> m_channels;
	Options m_options;
	State m_state;

	QPointer<GraphModel> m_model;
};

} // namespace timeline
