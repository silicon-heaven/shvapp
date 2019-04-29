#pragma once

#include "graph.h"
#include "sample.h"

#include <shv/coreqt/utils.h>

#include <QPointer>
#include <QScrollArea>
#include <QVector>
#include <QVariantMap>

namespace timeline {

class GraphModel;

class GraphWidget : public QWidget
{
	Q_OBJECT

	using Super = QWidget;
	//friend class GraphWidget;
public:
	using timemsec_t = Sample::timemsec_t;
	class GraphStyle : public QVariantMap
	{
		SHV_VARIANTMAP_FIELD2(double, l, setL, eftMargin, 0.3) // units
		SHV_VARIANTMAP_FIELD2(double, r, setR, ightMargin, 0.3) // units
		SHV_VARIANTMAP_FIELD2(double, t, setT, opMargin, 0.3) // units
		SHV_VARIANTMAP_FIELD2(double, b, setb, ottomMargin, 0.3) // units
		SHV_VARIANTMAP_FIELD2(double, c, setC, hannelSpacing, 0.3) // units
		SHV_VARIANTMAP_FIELD2(double, x, setX, AxisHeight, 1) // units
		SHV_VARIANTMAP_FIELD2(double, y, setY, AxisWidth, 2) // units
		SHV_VARIANTMAP_FIELD2(double, m, setM, iniMapHeight, 2) // units
		SHV_VARIANTMAP_FIELD2(double, v, setV, erticalHeaderWidth, 6) // units
		SHV_VARIANTMAP_FIELD2(bool, s, setS, eparateChannels, true)

		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olor, QColor(Qt::yellow))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorBackground, QColor(Qt::darkGray).darker(400))

		SHV_VARIANTMAP_FIELD(QFont, f, setF, ont)
	};
	class ChannelStyle : public QVariantMap
	{
	public:
		struct Interpolation { enum Enum {Line = 0, Stepped};};
		struct LineAreaStyle { enum Enum {Unfilled = 0, Filled};};
		static constexpr double CosmeticLineWidth = 0;

		SHV_VARIANTMAP_FIELD2(double, h, setH, eightMin, 2) // units
		SHV_VARIANTMAP_FIELD2(double, h, setH, eightMax, 2) // units
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olor, QColor(Qt::magenta))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorGrid, QColor(Qt::darkGreen))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorYAxis, QColor(Qt::green))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorBackground, QColor(Qt::black))

		SHV_VARIANTMAP_FIELD2(int, i, setI, nterpolation, Interpolation::Line)
		SHV_VARIANTMAP_FIELD2(int, l, setL, ineAreaStyle, LineAreaStyle::Unfilled)
		SHV_VARIANTMAP_FIELD2(double, l, setL, ineWidth, CosmeticLineWidth)

		//SHV_VARIANTMAP_FIELD(QFont, f, setF, ont)

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

		void setYRange(QPair<double, double> r);
		void enlargeYRange(double step);
		QPair<double, double> yRange() const { return QPair<double, double>{state.yRangeMin, state.yRangeMax}; }
		QPair<double, double> yRangeZoom() const { return QPair<double, double>{state.yRangeZoomMin, state.yRangeZoomMax}; }
		void resetZoom();

		ChannelStyle style;

		struct
		{
			double yRangeMin = 0;
			double yRangeMax = 0;
			double yRangeZoomMin = 0;
			double yRangeZoomMax = 0;
		} state;
	};
public:
	GraphWidget(QWidget *parent = nullptr);

	void setModel(GraphModel *model);
	GraphModel *model();
	void createChannelsFromModel();

	void clearChannels();
	void appendChannel();
	Channel& channelAt(int ix);
	const Channel& channelAt(int ix) const;

	QPair<timemsec_t, timemsec_t> xRange() const { return QPair<timemsec_t, timemsec_t>{state.xRangeMin, state.xRangeMax}; }
	QPair<timemsec_t, timemsec_t> xRangeZoom() const { return QPair<timemsec_t, timemsec_t>{state.xRangeZoomMin, state.xRangeZoomMax}; }
	void setXRange(QPair<timemsec_t, timemsec_t> r);

	void setStyle(const GraphStyle &st);
	void setDefaultChannelStyle(const ChannelStyle &st);

	void makeLayout(const QRect &rect);
protected:
	struct DrawChannelOptions
	{
		ChannelStyle style;

		struct
		{
			QRect rect;
			QRect graphRect;
			QRect verticalHeaderRect;
			QRect yAxisRect;
		} layout;

		DrawChannelOptions() {}
		DrawChannelOptions(const ChannelStyle &s)
			: style(s)
		{}

		//QFont font() const { return graphOptions.style.font(); }
	};

	struct DrawGraphOptions
	{
		GraphStyle style;
		int unitSize = 0;

		struct
		{
			QRect rect;
			QRect miniMapRect;
			QRect xAxisRect;
		} layout;

		QVector<DrawChannelOptions> channelOptions;

		DrawGraphOptions() {}
		DrawGraphOptions(const GraphStyle &s, int unit_size)
			: style(s)
			, unitSize(unit_size)
		{
		}

		int unitToPixels(double units) const { return static_cast<int>(unitSize * units); }
		QFont font() const { return style.font(); }

	};

	void makeLayout(const QRect &rect, DrawGraphOptions &opts);

	void drawRectText(QPainter *painter, const QRect &rect, const QString &text, const QFont &font, const QColor &color, const QColor &background = QColor());

	virtual void draw(QPainter *painter, const QRect &rect, const DrawGraphOptions &options);

	virtual void drawBackground(QPainter *painter, const QRect &rect, const DrawGraphOptions &options);
	virtual void drawMiniMap(QPainter *painter, const QRect &rect, const DrawGraphOptions &gr_options);
	virtual void drawXAxis(QPainter *painter, const QRect &rect, const DrawGraphOptions &options);

	//virtual void drawGraph(int channel);
	virtual void drawVerticalHeader(QPainter *painter, const QRect &rect, int channel, const DrawGraphOptions &options);
	virtual void drawBackground(QPainter *painter, const QRect &rect, int channel, const DrawGraphOptions &options);
	virtual void drawGrid(QPainter *painter, const QRect &rect, int channel, const DrawGraphOptions &options);
	virtual void drawYAxis(QPainter *painter, const QRect &rect, int channel, const DrawGraphOptions &options);
	virtual void drawSamples(QPainter *painter, const QRect &rect, int channel, const DrawGraphOptions &options);
	//virtual void drawCrossbar(QPainter *painter, const QRect &rect, int channel, const DrawGraphOptions &options);

	QVariantMap mergeMaps(const QVariantMap &base, const QVariantMap &overlay) const;

	// QWidget interface
protected:
	void paintEvent(QPaintEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;

	std::function<QPoint (const Sample&)> createSampleToPointFn(timemsec_t t1, timemsec_t t2, double d1, double d2, const QRect &dest_rect);
	std::function<Sample (const QPoint &)> createPointToSampleFn(const QRect &src_rect, timemsec_t t1, timemsec_t t2, double d1, double d2);
protected:
	GraphStyle m_style;
	ChannelStyle m_channelStyle;

	QVector<Channel> m_channels;

	//double xRange() const { return xRangeMax - xRangeMin; }

	struct State
	{
		timemsec_t xRangeMin = 0;
		timemsec_t xRangeMax = 0;
		timemsec_t xRangeZoomMin = 0;
		timemsec_t xRangeZoomMax = 0;
	} state;

	QPointer<GraphModel> m_model;

	GraphWidget::DrawGraphOptions m_drawOptions;
};

} // namespace timeline
