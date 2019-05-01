#pragma once

#include "sample.h"

#include <shv/coreqt/utils.h>

#include <QPointer>
#include <QVector>
#include <QVariantMap>
#include <QColor>
#include <QFont>
#include <QRect>

namespace timeline {

class GraphModel;

class Graph
{
public:
	using timemsec_t = Sample::timemsec_t;

	struct XRange
	{
		timemsec_t min = 0;
		timemsec_t max = 0;

		XRange() {}
		XRange(timemsec_t mn, timemsec_t mx) : min(mn), max(mx) {}
		XRange(const QPair<timemsec_t, timemsec_t> r) : min(r.first), max(r.second) {}

		bool isNull() const { return min == 0 && max == 0; }
	};

	struct YRange
	{
		double min = 0;
		double max = 0;

		YRange() {}
		YRange(double mn, double mx) : min(mn), max(mx) {}
		YRange(const QPair<double, double> r) : min(r.first), max(r.second) {}

		bool isNull() const { return min == 0 && max == 0; }
	};

	struct DataRect
	{
		XRange xRange;
		YRange yRange;

		DataRect() {}
		DataRect(const XRange &xr, const YRange &yr) : xRange(xr), yRange(yr) {}

		bool isNull() const { return xRange.isNull() && yRange.isNull(); }
	};

	class GraphStyle : public QVariantMap
	{
		SHV_VARIANTMAP_FIELD2(int, u, setU, nitSize, 20) // px
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
	public:
		void init(QWidget *widget);
	};
	class ChannelStyle : public QVariantMap
	{
	public:
		struct Interpolation { enum Enum {None = 0, Line, Stepped};};
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

	class Channel
	{
		//int metaTypeId = 0;
	public:
		void setYRange(const YRange &r);
		void enlargeYRange(double step);
		YRange yRange() const { return m_state.yRange; }
		YRange yRangeZoom() const { return m_state.yRangeZoom; }
		void resetZoom();

		const ChannelStyle& style() const {return m_style;}
		void setStyle(const ChannelStyle& st) { m_style = st; }
		ChannelStyle effectiveStyle;

		const QRect& graphRect() const { return  m_layout.graphRect; }

		//std::function<QPoint (const Sample&)> sampleToPointFn(const XRange &xrange);
		//std::function<Sample (const QPoint &)> pointToSampleFn(const XRange &xrange);

		struct
		{
			YRange yRange;
			YRange yRangeZoom;
		} m_state;

		struct
		{
			//QRect rect;
			QRect graphRect;
			QRect verticalHeaderRect;
			QRect yAxisRect;
		} m_layout;
	protected:
		ChannelStyle m_style;
	};
public:
	Graph();
	virtual ~Graph() {}

	void setModel(GraphModel *model);
	GraphModel *model() const;

	void createChannelsFromModel();

	int channelCount() const { return  m_channels.count(); }
	void clearChannels();
	void appendChannel();
	Channel& channelAt(int ix);
	const Channel& channelAt(int ix) const;
	DataRect dataRect(int channel_ix) const;

	timemsec_t miniMapPosToTime(int pos) const;
	int miniMapTimeToPos(timemsec_t time) const;

	timemsec_t posToTime(int pos) const;
	int timeToPos(timemsec_t time) const;
	Sample timeToSample(int channel_ix, timemsec_t time) const;

	const QRect& rect() const { return  m_layout.rect; }
	const QRect& miniMapRect() const { return  m_layout.miniMapRect; }
	int setCrossBarPos(const QPoint &pos);

	XRange xRange() const { return m_state.xRange; }
	XRange xRangeZoom() const { return m_state.xRangeZoom; }
	void setXRange(const XRange &r, bool keep_zoom = false);
	void setXRangeZoom(const XRange &r);

	const GraphStyle& style() const { return m_style; }
	void setStyle(const GraphStyle &st);
	void setDefaultChannelStyle(const ChannelStyle &st);

	void makeLayout(const QRect &rect);
	void draw(QPainter *painter);
	int u2px(double u) const;

	static std::function<QPoint (const Sample&)> sampleToPointFn(const DataRect &src, const QRect &dest);
	static std::function<Sample (const QPoint &)> pointToSampleFn(const QRect &src, const DataRect &dest);
	static std::function<timemsec_t (int)> posToTimeFn(const QPoint &src, const XRange &dest);
	static std::function<int (timemsec_t)> timeToPosFn(const XRange &src, const QPoint &dest);

protected:
	void sanityXRangeZoom();

	void drawRectText(QPainter *painter, const QRect &rect, const QString &text, const QFont &font, const QColor &color, const QColor &background = QColor());

	virtual void drawBackground(QPainter *painter);
	virtual void drawMiniMap(QPainter *painter);
	virtual void drawXAxis(QPainter *painter);

	//virtual void drawGraph(int channel);
	virtual void drawVerticalHeader(QPainter *painter, int channel);
	virtual void drawBackground(QPainter *painter, int channel);
	virtual void drawGrid(QPainter *painter, int channel);
	virtual void drawYAxis(QPainter *painter, int channel);
	virtual void drawSamples(QPainter *painter, int channel
			, const DataRect &src_rect = DataRect()
			, const QRect &dest_rect = QRect()
			, const ChannelStyle &channel_style = ChannelStyle());
	//virtual void drawCrossbar(QPainter *painter, const QRect &rect, int channel, const DrawGraphOptions &options);

	QVariantMap mergeMaps(const QVariantMap &base, const QVariantMap &overlay) const;
protected:
	QPointer<GraphModel> m_model;

	GraphStyle effectiveStyle;

	GraphStyle m_style;
	ChannelStyle m_channelStyle;

	QVector<Channel> m_channels;

	struct
	{
		XRange xRange;
		XRange xRangeZoom;
		QPoint crossBarPos;
	} m_state;

	struct
	{
		QRect rect;
		QRect miniMapRect;
		QRect xAxisRect;
	} m_layout;
};

} // namespace timeline
