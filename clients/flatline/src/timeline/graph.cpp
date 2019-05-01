#include "graphwidget.h"
#include "graphmodel.h"
#include "graphwidget.h"

#include <shv/core/exception.h>
#include <shv/coreqt/log.h>

#include <QPainter>
#include <QFontMetrics>
#include <QLabel>
#include <QMouseEvent>

#include <cmath>

namespace timeline {

void Graph::GraphStyle::init(QWidget *widget)
{
	QFont f = widget->font();
	setFont(f);
	QFontMetrics fm(f, widget);
	setUnitSize(fm.lineSpacing());
}

Graph::Graph()
{
}

void Graph::setModel(GraphModel *model)
{
	m_model = model;
}

GraphModel *Graph::model()
{
	return m_model;
}

void Graph::createChannelsFromModel()
{
	m_channels.clear();
	if(m_model.isNull())
		return;
	for (int i = 0; i < m_model->channelCount(); ++i) {
		m_channels.append(Channel());
	}
}

void Graph::clearChannels()
{
	m_channels.clear();
}

void Graph::appendChannel()
{
	m_channels.append(Channel());
}

Graph::Channel &Graph::channelAt(int ix)
{
	if(ix < 0 || ix >= m_channels.count())
		SHV_EXCEPTION("Index out of range.");
	return m_channels[ix];
}

const Graph::Channel &Graph::channelAt(int ix) const
{
	if(ix < 0 || ix >= m_channels.count())
		SHV_EXCEPTION("Index out of range.");
	return m_channels[ix];
}

Graph::DataRect Graph::dataRect(int channel_ix) const
{
	const Channel &ch = channelAt(channel_ix);
	return DataRect{xRangeZoom(), ch.yRangeZoom()};
}

void Graph::setXRange(const timeline::Graph::XRange &r)
{
	m_state.xRange = r;
	if(m_state.xRangeZoom.isNull()) {
		m_state.xRangeZoom = m_state.xRange;
	}
	if(m_state.xRangeZoom.min < m_state.xRange.min)
		m_state.xRangeZoom.min = m_state.xRange.min;
	if(m_state.xRangeZoom.max > m_state.xRange.max)
		m_state.xRangeZoom.max = m_state.xRange.max;
}

void Graph::Channel::setYRange(const timeline::Graph::YRange &r)
{
	m_state.yRange = r;
	resetZoom();
}

void Graph::Channel::enlargeYRange(double step)
{
	YRange r = m_state.yRange;
	r.min -= step;
	r.max += step;
	setYRange(r);
}

void Graph::Channel::resetZoom()
{
	m_state.yRangeZoom = m_state.yRange;
}

void Graph::setStyle(const Graph::GraphStyle &st)
{
	m_style = st;
	effectiveStyle = m_style;
}

void Graph::setDefaultChannelStyle(const Graph::ChannelStyle &st)
{
	m_channelStyle = st;
}

QVariantMap Graph::mergeMaps(const QVariantMap &base, const QVariantMap &overlay) const
{
	QVariantMap ret = base;
	QMapIterator<QString, QVariant> it(overlay);
	//shvDebug() << "====================== merge ====================";
	while(it.hasNext()) {
		it.next();
		//shvDebug() << it.key() << "<--" << it.value();
		ret[it.key()] = it.value();
	}
	return ret;
}

int Graph::u2px(double u) const
{
	int sz = effectiveStyle.unitSize();
	return static_cast<int>(sz * u);
}

void Graph::makeLayout(const QRect &rect)
{
	QSize viewport_size;
	viewport_size.setWidth(rect.width());
	int grid_w = viewport_size.width();
	int x_axis_pos = 0;
	x_axis_pos += u2px(effectiveStyle.leftMargin());
	x_axis_pos += u2px(effectiveStyle.verticalHeaderWidth());
	x_axis_pos += u2px(effectiveStyle.yAxisWidth());
	grid_w -= x_axis_pos;
	grid_w -= u2px(effectiveStyle.rightMargin());
	m_layout.miniMapRect.setHeight(u2px(effectiveStyle.miniMapHeight()));
	m_layout.miniMapRect.setLeft(x_axis_pos);
	m_layout.miniMapRect.setWidth(grid_w);

	m_layout.xAxisRect = m_layout.miniMapRect;
	m_layout.xAxisRect.setHeight(u2px(effectiveStyle.xAxisHeight()));

	int sum_h_min = 0;
	struct Rest { int index; int rest; };
	QVector<Rest> rests;
	for (int i = 0; i < m_channels.count(); ++i) {
		Channel &ch = m_channels[i];
		ch.effectiveStyle = mergeMaps(m_channelStyle, ch.style());
		int ch_h = u2px(ch.effectiveStyle.heightMin());
		rests << Rest{i, u2px(ch.effectiveStyle.heightRange())};
		ch.m_layout.graphRect.setLeft(x_axis_pos);
		ch.m_layout.graphRect.setWidth(grid_w);
		ch.m_layout.graphRect.setHeight(ch_h);
		sum_h_min += ch_h;
		if(i > 0)
			sum_h_min += u2px(effectiveStyle.channelSpacing());
	}
	sum_h_min += u2px(effectiveStyle.xAxisHeight());
	sum_h_min += u2px(effectiveStyle.miniMapHeight());
	int h_rest = rect.height();
	h_rest -= sum_h_min;
	h_rest -= u2px(effectiveStyle.topMargin());
	h_rest -= u2px(effectiveStyle.bottomMargin());
	if(h_rest > 0) {
		// distribute free widget height space to channel's rects heights
		std::sort(rests.begin(), rests.end(), [](const Rest &a, const Rest &b) {
			return a.rest < b.rest;
		});
		for (int i = 0; i < rests.count(); ++i) {
			int fair_rest = h_rest / (rests.count() - i);
			const Rest &r = rests[i];
			Channel &ch = m_channels[r.index];
			int h = u2px(ch.effectiveStyle.heightRange());
			if(h > fair_rest)
				h = fair_rest;
			ch.m_layout.graphRect.setHeight(ch.m_layout.graphRect.height() + h);
			h_rest -= h;
		}
	}
	// shift channel rects
	int widget_height = 0;
	widget_height += u2px(effectiveStyle.topMargin());
	for (int i = m_channels.count() - 1; i >= 0; --i) {
		Channel &ch = m_channels[i];

		ch.m_layout.graphRect.moveTop(widget_height);
		//ch.layout.graphRect.setWidth(layout.navigationBarRect.width());

		ch.m_layout.verticalHeaderRect = ch.m_layout.graphRect;
		ch.m_layout.verticalHeaderRect.setX(u2px(effectiveStyle.leftMargin()));
		ch.m_layout.verticalHeaderRect.setWidth(u2px(effectiveStyle.verticalHeaderWidth()));

		ch.m_layout.yAxisRect = ch.m_layout.verticalHeaderRect;
		ch.m_layout.yAxisRect.moveLeft(ch.m_layout.verticalHeaderRect.right());
		ch.m_layout.yAxisRect.setWidth(u2px(effectiveStyle.yAxisWidth()));

		widget_height += ch.m_layout.graphRect.height();
		if(i > 0)
			widget_height += u2px(effectiveStyle.channelSpacing());
	}
	m_layout.xAxisRect.moveTop(widget_height);
	widget_height += u2px(effectiveStyle.xAxisHeight());
	m_layout.miniMapRect.moveTop(widget_height);
	widget_height += u2px(effectiveStyle.miniMapHeight());
	widget_height += u2px(effectiveStyle.bottomMargin());

	viewport_size.setHeight(widget_height);
	shvDebug() << "\tw:" << viewport_size.width() << "h:" << viewport_size.height();
	m_layout.rect = rect;
	m_layout.rect.setSize(viewport_size);
}

void Graph::drawRectText(QPainter *painter, const QRect &rect, const QString &text, const QFont &font, const QColor &color, const QColor &background)
{
	painter->save();
	if(background.isValid())
		painter->fillRect(rect, background);
	QPen pen;
	pen.setColor(color);
	painter->setPen(pen);
	painter->drawRect(rect);
	painter->setFont(font);
	QFontMetrics fm(font);
	painter->drawText(rect.left() + fm.width('i'), rect.top() + fm.leading() + fm.ascent(), text);
	painter->restore();
}

void Graph::draw(QPainter *painter)
{
	shvLogFuncFrame();
	drawBackground(painter);
	for (int i = 0; i < m_channels.count(); ++i) {
		const Channel &ch = m_channels[i];
		drawVerticalHeader(painter, i);
		drawYAxis(painter, i);
		drawBackground(painter, i);
		drawGrid(painter, i);
		drawSamples(painter, i);
	}
	drawMiniMap(painter);
	drawXAxis(painter);
}

void Graph::drawBackground(QPainter *painter)
{
	painter->fillRect(m_layout.rect, effectiveStyle.colorBackground());
	//painter->fillRect(QRect{QPoint(), widget()->geometry().size()}, Qt::blue);
}

void Graph::drawMiniMap(QPainter *painter)
{
	painter->fillRect(m_layout.miniMapRect, m_channelStyle.colorBackground());
	for (int i = 0; i < m_channels.count(); ++i) {
		const Channel &ch = m_channels[i];
		ChannelStyle ch_st = ch.effectiveStyle;
		ch_st.setLineAreaStyle(ChannelStyle::LineAreaStyle::Filled);
		drawSamples(painter, i, m_layout.miniMapRect, ch_st);
	}
}

void Graph::drawXAxis(QPainter *painter)
{
	drawRectText(painter, m_layout.xAxisRect, "x-axis", effectiveStyle.font(), Qt::green);
}

void Graph::drawVerticalHeader(QPainter *painter, int channel)
{
	const Channel &ch = m_channels[channel];
	QColor c = effectiveStyle.color();
	QColor bc = c;
	bc.setAlphaF(0.05);
	QString text = model()->channelData(channel, GraphModel::ChannelDataRole::Name).toString();
	if(text.isEmpty())
		text = "<no name>";
	QFont font = effectiveStyle.font();
	font.setBold(true);
	drawRectText(painter, ch.m_layout.verticalHeaderRect, text, font, c, bc);
}

void Graph::drawBackground(QPainter *painter, int channel)
{
	const Channel &ch = m_channels[channel];
	painter->fillRect(ch.m_layout.graphRect, ch.effectiveStyle.colorBackground());
}

void Graph::drawGrid(QPainter *painter, int channel)
{
	const Channel &ch = m_channels[channel];
	drawRectText(painter, ch.m_layout.graphRect, "grid", effectiveStyle.font(), ch.effectiveStyle.colorGrid());
}

void Graph::drawYAxis(QPainter *painter, int channel)
{
	const Channel &ch = m_channels[channel];
	drawRectText(painter, ch.m_layout.yAxisRect, "y-axis", effectiveStyle.font(), ch.effectiveStyle.colorYAxis());
}

std::function<QPoint (const Sample &)> Graph::createSampleToPointFn(const DataRect &src, const QRect &dest)
{
	int le = dest.left();
	int ri = dest.right();
	int to = dest.top();
	int bo = dest.bottom();

	timemsec_t t1 = src.xRange.min;
	timemsec_t t2 = src.xRange.max;
	double d1 = src.yRange.min;
	double d2 = src.yRange.max;

	if(t2 - t1 == 0)
		return nullptr;
	double kx = static_cast<double>(ri - le) / (t2 - t1);
	//shvDebug() << d1 << d2;
	if(std::abs(d2 - d1) < 1e-6)
		return nullptr;
	double ky = (to - bo) / (d2 - d1);

	return  [le, bo, kx, t1, d1, ky](const Sample &s) -> QPoint {
		const timemsec_t t = s.time;
		double d = GraphModel::valueToDouble(s.value);
		double x = le + (t - t1) * kx;
		double y = bo + (d - d1) * ky;
		return QPoint{static_cast<int>(x), static_cast<int>(y)};
	};
}

std::function<Sample (const QPoint &)> Graph::createPointToSampleFn(const QRect &src, const DataRect &dest)
{
	int le = src.left();
	int ri = src.right();
	int to = src.top();
	int bo = src.bottom();

	if(ri - le == 0)
		return nullptr;

	timemsec_t t1 = dest.xRange.min;
	timemsec_t t2 = dest.xRange.max;
	double d1 = dest.yRange.min;
	double d2 = dest.yRange.max;

	double kx = static_cast<double>(t2 - t1) / (ri - le);
	if(to - bo == 0)
		return nullptr;
	double ky = (d2 - d1) / (to - bo);

	return  [t1, le, kx, d1, bo, ky](const QPoint &p) -> Sample {
		const int x = p.x();
		const int y = p.y();
		timemsec_t t = static_cast<timemsec_t>(t1 + (x - le) * kx);
		double d = d1 + (y - bo) * ky;
		return Sample{t, d};
	};
}

void Graph::drawSamples(QPainter *painter, int channel, const QRect &dest_rect, const Graph::ChannelStyle &channel_style)
{
	//shvLogFuncFrame() << "channel:" << channel;
	const Channel &ch = channelAt(channel);
	QRect rect = dest_rect.isEmpty()? ch.m_layout.graphRect: dest_rect;
	ChannelStyle ch_style = channel_style.isEmpty()? ch.effectiveStyle: channel_style;

	XRange xrange = xRangeZoom();
	YRange yrange = ch.yRangeZoom();
	/*
	timemsec_t t1 = xrange.min;
	timemsec_t t2 = xrange.max;
	double d1 = yrange.min;
	double d2 = yrange.max;
	*/
	auto sample2point = createSampleToPointFn(DataRect{xrange, yrange}, rect);

	if(!sample2point)
		return;

	int interpolation = ch_style.interpolation();
	painter->save();
	QPen pen;
	pen.setColor(ch_style.color());
	{
		double d = ch_style.lineWidth();
		if(d > 0)
			pen.setWidth(u2px(d));
		else
			pen.setWidth(2);
	}
	painter->setPen(pen);
	QColor line_area_color;
	if(ch_style.lineAreaStyle() == ChannelStyle::LineAreaStyle::Filled) {
		line_area_color = painter->pen().color();
		line_area_color.setAlphaF(0.4);
		//line_area_color.setHsv(line_area_color.hslHue(), line_area_color.hsvSaturation() / 2, line_area_color.lightness());
	}

	GraphModel *m = model();
	QPoint p1;
	//shvDebug() << "count:" << m->count(channel);
	for (int i = 0; i < m->count(channel); ++i) {
		const Sample s = m->sampleAt(channel, i);
		const QPoint p2 = sample2point(s);
		if(i == 0) {
			p1 = p2;
		}
		else {
			if(p2.x() != p1.x()) {
				if(interpolation == ChannelStyle::Interpolation::Stepped) {
					if(line_area_color.isValid()) {
						const QPoint p0 = sample2point(Sample{s.time, 0});
						painter->fillRect(QRect{p1 + QPoint{1, 0}, p0}, line_area_color);
					}
					QPoint p3{p2.x(), p1.y()};
					painter->drawLine(p1, p3);
					painter->drawLine(p3, p2);
				}
				else {
					if(line_area_color.isValid()) {
						const QPoint p0 = sample2point(Sample{s.time, 0});
						QPainterPath pp;
						pp.moveTo(p1);
						pp.lineTo(p2);
						pp.lineTo(p0);
						pp.lineTo(p1.x(), p0.y());
						pp.closeSubpath();
						painter->fillPath(pp, line_area_color);
					}
					painter->drawLine(p1, p2);
				}
				p1 = p2;
			}
		}
	}
	painter->restore();
}

} // namespace timeline
