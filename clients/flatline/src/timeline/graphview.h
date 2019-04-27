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
		SHV_VARIANTMAP_FIELD2(double, r, setR, ightMargin, 0.3) // units
		SHV_VARIANTMAP_FIELD2(double, t, setT, opMargin, 0.3) // units
		SHV_VARIANTMAP_FIELD2(double, b, setb, ottomMargin, 0.3) // units
		SHV_VARIANTMAP_FIELD2(double, c, setC, hannelSpacing, 0.3) // units
		SHV_VARIANTMAP_FIELD2(double, x, setX, AxisHeight, 1) // units
		SHV_VARIANTMAP_FIELD2(double, y, setY, AxisWidth, 2) // units
		SHV_VARIANTMAP_FIELD2(double, n, setN, avigationBarHeight, 2) // units
		SHV_VARIANTMAP_FIELD2(double, v, setV, erticalHeaderWidth, 6) // units
		SHV_VARIANTMAP_FIELD2(bool, s, setS, eparateChannels, true)

		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olor, QColor(Qt::yellow))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorBackground, QColor(Qt::darkGray))
	};
	struct Interpolation { enum Enum {Line = 0, Stepped};};
	static constexpr double CosmeticLineWidth = 0;
	class ChannelStyle : public QVariantMap
	{
		SHV_VARIANTMAP_FIELD2(double, h, setH, eightMin, 2) // units
		SHV_VARIANTMAP_FIELD2(double, h, setH, eightMax, 2) // units
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olor, QColor(Qt::magenta))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorGrid, QColor(Qt::darkGreen))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorYAxis, QColor(Qt::green))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorBackground, QColor(Qt::black))
		SHV_VARIANTMAP_FIELD2(int, i, setI, nterpolation, Interpolation::Line)
		SHV_VARIANTMAP_FIELD2(double, l, setL, ineWidth, CosmeticLineWidth)

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

	struct Channel
	{
		int metaTypeId = 0;

		double yRangeMin = 0;
		double yRangeMax = 0;
		void setYRange(QPair<double, double> r) {yRangeMin = r.first; yRangeMax = r.second; }
		void adjustYRange(double step) { yRangeMin -= step; yRangeMax += step; }
		QPair<double, double> yRange() const { return QPair<double, double>{yRangeMin, yRangeMax}; }

		ChannelStyle style;

		struct
		{
			double yRangeDisplMin = 0;
			double yRangeDisplMax = 0;
		} state;

		struct
		{
			QRect graphRect;
			QRect verticalHeaderRect;
			QRect yAxisRect;
		} layout;
	};
public:
	GraphView(QWidget *parent = nullptr);

	GraphStyle style;
	ChannelStyle channelStyle;

	void setModel(GraphModel *model);
	GraphModel *model();
	void createChannelsFromModel();

	void clearChannels();
	void appendChannel();
	Channel& channelAt(int ix);
	const Channel& channelAt(int ix) const;

	QPair<timemsec_t, timemsec_t> xRange() const { return QPair<timemsec_t, timemsec_t>{xRangeMin, xRangeMax}; }
	void setXRange(QPair<timemsec_t, timemsec_t> r) {xRangeMin = r.first; xRangeMax = r.second; }

	void makeLayout();
protected:
	QVariantMap mergeMaps(const QVariantMap &base, const QVariantMap &overlay) const;
	void makeLayout(int unit_size, const QSize &widget_size);

	void drawRectText(QPainter *painter, const QRect &rect, const QString &text, const QFont &font, const QColor &color, const QColor &background = QColor());

	struct GraphDrawOptions
	{
		QFont font;
		int unitSize = 0;

		GraphDrawOptions(const QFont &f, int unit_size) : font(f), unitSize(unit_size) {}

		int unitToPixels(double units) const { return static_cast<int>(unitSize * units); }

	};
	virtual void draw(QPainter *painter, const GraphDrawOptions &options);

	virtual void drawBackground(QPainter *painter, const GraphDrawOptions &options);
	virtual void drawNavigationBar(QPainter *painter, const GraphDrawOptions &options);
	virtual void drawXAxis(QPainter *painter, const GraphDrawOptions &options);

	struct DrawChannelOptions
	{
		GraphDrawOptions graphOptions;
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
	void resizeEvent(QResizeEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
protected:
	QVector<Channel> m_channels;

	timemsec_t xRangeMin = 0;
	timemsec_t xRangeMax = 0;
	//double xRange() const { return xRangeMax - xRangeMin; }

	struct State
	{
		timemsec_t xRangeDisplMin = 0;
		timemsec_t xRangeDisplMax = 0;
	} state;

	struct
	{
		QRect navigationBarRect;
		QRect xAxisRect;
		int unitSize = 0;
	} layout;

	QPointer<GraphModel> m_model;
};

} // namespace timeline
