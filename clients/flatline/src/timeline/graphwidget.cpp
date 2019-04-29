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

GraphWidget::GraphWidget(QWidget *parent)
	: Super(parent)
{
	m_style.setFont(font());
	setMouseTracking(true);
	//setBackgroundRole(QPalette::Dark);
	//setWidget(new GraphWidget(this));
	//init();
	//m_style.setFont(font());
	//channelStyle.setFont(style.font());
}
/*
void GraphView::init()
{
	setBackgroundRole(QPalette::Dark);
	QLabel *lbl = new QLabel("ahoj");
	QImage image("/home/fanda/t/1.png");
	lbl->setPixmap(QPixmap::fromImage(image));
	lbl->setMinimumSize({100, 100});
	setWidget(lbl);
	shvInfo() << qobject_cast<QWidget*>(lbl) << qobject_cast<QWidget*>(widget());
}
*/
void GraphWidget::setModel(GraphModel *model)
{
	m_model = model;
}

GraphModel *GraphWidget::model()
{
	return m_model;
}

void GraphWidget::createChannelsFromModel()
{
	m_channels.clear();
	GraphModel *m = model();
	for (int i = 0; i < m->channelCount(); ++i) {
		m_channels.append(Channel());
	}
}

void GraphWidget::clearChannels()
{
	m_channels.clear();
}

void GraphWidget::appendChannel()
{
	m_channels.append(Channel());
}

GraphWidget::Channel &GraphWidget::channelAt(int ix)
{
	if(ix < 0 || ix >= m_channels.count())
		SHV_EXCEPTION("Index out of range.");
	return m_channels[ix];
}

const GraphWidget::Channel &GraphWidget::channelAt(int ix) const
{
	if(ix < 0 || ix >= m_channels.count())
		SHV_EXCEPTION("Index out of range.");
	return m_channels[ix];
}

void GraphWidget::setXRange(QPair<GraphWidget::timemsec_t, GraphWidget::timemsec_t> r)
{
	state.xRangeMin = r.first;
	state.xRangeMax = r.second;
	if(state.xRangeZoomMin == 0 && state.xRangeZoomMax == 0) {
		state.xRangeZoomMin = state.xRangeMin;
		state.xRangeZoomMax = state.xRangeMax;
	}
	if(state.xRangeZoomMin < state.xRangeMin)
		state.xRangeZoomMin = state.xRangeMin;
	if(state.xRangeZoomMax > state.xRangeMax)
		state.xRangeZoomMax = state.xRangeMax;
}

void GraphWidget::Channel::setYRange(QPair<double, double> r)
{
	state.yRangeMin = r.first;
	state.yRangeMax = r.second;
	resetZoom();
}

void GraphWidget::Channel::enlargeYRange(double step)
{
	QPair<double, double> r = yRange();
	r.first -= step;
	r.second += step;
	setYRange(r);
}

void GraphWidget::Channel::resetZoom()
{
	state.yRangeZoomMin = state.yRangeMin;
	state.yRangeZoomMax = state.yRangeMax;
}

void GraphWidget::setStyle(const GraphWidget::GraphStyle &st)
{
	m_style = st;
	if(!st.font_isset())
		m_style.setFont(font());
}

void GraphWidget::setDefaultChannelStyle(const GraphWidget::ChannelStyle &st)
{
	m_channelStyle = st;
}

QVariantMap GraphWidget::mergeMaps(const QVariantMap &base, const QVariantMap &overlay) const
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

void GraphWidget::makeLayout(const QRect &rect)
{
	shvLogFuncFrame();
	static int unit_size = 0;
	if(unit_size == 0) {
		QFontMetrics fm(font());
		unit_size = fm.lineSpacing();
	}
	m_drawOptions = DrawGraphOptions(m_style, unit_size);
	makeLayout(rect, m_drawOptions);
	setMinimumSize(m_drawOptions.layout.rect.size());
	setMaximumSize(m_drawOptions.layout.rect.size());
	update();
}

void GraphWidget::makeLayout(const QRect &rect, DrawGraphOptions &opts)
{
	auto u2px = [opts](double unit) -> int { return static_cast<int>(opts.unitSize * unit); };
	QSize viewport_size;
	viewport_size.setWidth(rect.width());
	int grid_w = viewport_size.width();
	int x_axis_pos = 0;
	x_axis_pos += u2px(opts.style.leftMargin());
	x_axis_pos += u2px(opts.style.verticalHeaderWidth());
	x_axis_pos += u2px(opts.style.yAxisWidth());
	grid_w -= x_axis_pos;
	grid_w -= u2px(opts.style.rightMargin());
	opts.layout.miniMapRect.setHeight(u2px(opts.style.miniMapHeight()));
	opts.layout.miniMapRect.setLeft(x_axis_pos);
	opts.layout.miniMapRect.setWidth(grid_w);

	opts.layout.xAxisRect = opts.layout.miniMapRect;
	opts.layout.xAxisRect.setHeight(u2px(opts.style.xAxisHeight()));

	opts.channelOptions.resize(m_channels.count());

	int sum_h_min = 0;
	struct Rest { int index; int rest; };
	QVector<Rest> rests;
	for (int i = 0; i < m_channels.count(); ++i) {
		Channel &ch = m_channels[i];
		DrawChannelOptions &ch_opts = opts.channelOptions[i];
		ch_opts.style = mergeMaps(m_channelStyle, ch.style);
		int ch_h = u2px(ch_opts.style.heightMin());
		rests << Rest{i, u2px(ch_opts.style.heightRange())};
		ch_opts.layout.graphRect.setLeft(x_axis_pos);
		ch_opts.layout.graphRect.setWidth(grid_w);
		ch_opts.layout.graphRect.setHeight(ch_h);
		sum_h_min += ch_h;
		if(i > 0)
			sum_h_min += u2px(opts.style.channelSpacing());
	}
	sum_h_min += u2px(opts.style.xAxisHeight());
	sum_h_min += u2px(opts.style.miniMapHeight());
	int h_rest = rect.height();
	h_rest -= sum_h_min;
	h_rest -= u2px(opts.style.topMargin());
	h_rest -= u2px(opts.style.bottomMargin());
	if(h_rest > 0) {
		// distribute free widget height space to channel's rects heights
		std::sort(rests.begin(), rests.end(), [](const Rest &a, const Rest &b) {
			return a.rest < b.rest;
		});
		for (int i = 0; i < rests.count(); ++i) {
			int fair_rest = h_rest / (rests.count() - i);
			const Rest &r = rests[i];
			//Channel &ch = m_channels[r.index];
			DrawChannelOptions &ch_opts = opts.channelOptions[i];
			int h = u2px(ch_opts.style.heightRange());
			if(h > fair_rest)
				h = fair_rest;
			ch_opts.layout.graphRect.setHeight(ch_opts.layout.graphRect.height() + h);
			h_rest -= h;
		}
	}
	// shift channel rects
	int widget_height = 0;
	widget_height += u2px(opts.style.topMargin());
	for (int i = m_channels.count() - 1; i >= 0; --i) {
		Channel &ch = m_channels[i];
		DrawChannelOptions &ch_opts = opts.channelOptions[i];

		ch_opts.layout.graphRect.moveTop(widget_height);
		//ch.layout.graphRect.setWidth(layout.navigationBarRect.width());

		ch_opts.layout.verticalHeaderRect = ch_opts.layout.graphRect;
		ch_opts.layout.verticalHeaderRect.setX(u2px(opts.style.leftMargin()));
		ch_opts.layout.verticalHeaderRect.setWidth(u2px(opts.style.verticalHeaderWidth()));

		ch_opts.layout.yAxisRect = ch_opts.layout.verticalHeaderRect;
		ch_opts.layout.yAxisRect.moveLeft(ch_opts.layout.verticalHeaderRect.right());
		ch_opts.layout.yAxisRect.setWidth(u2px(opts.style.yAxisWidth()));

		widget_height += ch_opts.layout.graphRect.height();
		if(i > 0)
			widget_height += u2px(opts.style.channelSpacing());
	}
	opts.layout.xAxisRect.moveTop(widget_height);
	widget_height += u2px(opts.style.xAxisHeight());
	opts.layout.miniMapRect.moveTop(widget_height);
	widget_height += u2px(opts.style.miniMapHeight());
	widget_height += u2px(opts.style.bottomMargin());

	viewport_size.setHeight(widget_height);
	shvDebug() << "\tw:" << viewport_size.width() << "h:" << viewport_size.height();
	opts.layout.rect = rect;
	opts.layout.rect.setSize(viewport_size);
}

void GraphWidget::drawRectText(QPainter *painter, const QRect &rect, const QString &text, const QFont &font, const QColor &color, const QColor &background)
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

void GraphWidget::draw(QPainter *painter, const QRect &rect, const DrawGraphOptions &gr_options)
{
	shvLogFuncFrame();
	drawBackground(painter, rect, gr_options);
	for (int i = 0; i < m_channels.count(); ++i) {
		//const Channel &ch = m_channels[i];
		const DrawChannelOptions &ch_opts = gr_options.channelOptions[i];
		drawVerticalHeader(painter, ch_opts.layout.verticalHeaderRect, i, gr_options);
		drawYAxis(painter, ch_opts.layout.yAxisRect, i, gr_options);
		drawBackground(painter, ch_opts.layout.graphRect, i, gr_options);
		drawGrid(painter, ch_opts.layout.graphRect, i, gr_options);
		drawSamples(painter, ch_opts.layout.graphRect, i, gr_options);
	}
	drawMiniMap(painter, gr_options.layout.miniMapRect, gr_options);
	drawXAxis(painter, gr_options.layout.xAxisRect, gr_options);
}

void GraphWidget::drawBackground(QPainter *painter, const QRect &rect, const DrawGraphOptions &options)
{
	painter->fillRect(rect, options.style.colorBackground());
	//painter->fillRect(QRect{QPoint(), widget()->geometry().size()}, Qt::blue);
}

void GraphWidget::drawMiniMap(QPainter *painter, const QRect &rect, const DrawGraphOptions &gr_options)
{
	//drawRectText(painter, layout.miniMapRect, "navigation bar", options.font, Qt::blue);
	//DrawChannelOptions ch_options{gr_options, layout.miniMapRect, ch_style, style};
	DrawGraphOptions opts = gr_options;
	opts.style.setColorBackground(Qt::black);
	drawBackground(painter, opts.layout.miniMapRect, opts);
	for (int i = 0; i < m_channels.count(); ++i) {
		//const Channel &ch = m_channels[i];
		DrawChannelOptions &ch_opts = opts.channelOptions[i];
		ch_opts.style.setLineAreaStyle(ChannelStyle::LineAreaStyle::Filled);
		drawSamples(painter, rect, i, opts);
	}
}

void GraphWidget::drawXAxis(QPainter *painter, const QRect &rect, const DrawGraphOptions &options)
{
	drawRectText(painter, rect, "x-axis", options.style.font(), Qt::green);
}

void GraphWidget::drawVerticalHeader(QPainter *painter, const QRect &rect, int channel, const DrawGraphOptions &options)
{
	//const Channel &ch = m_channels[channel];
	QColor c = options.style.color();
	QColor bc = c;
	bc.setAlphaF(0.05);
	QString text = model()->channelData(channel, GraphModel::ChannelDataRole::Name).toString();
	if(text.isEmpty())
		text = "<no name>";
	QFont font = options.font();
	font.setBold(true);
	drawRectText(painter, rect, text, font, c, bc);
}

void GraphWidget::drawBackground(QPainter *painter, const QRect &rect, int channel, const DrawGraphOptions &options)
{
	Q_UNUSED(channel)
	//const Channel &ch = m_channels[channel];
	painter->fillRect(rect, options.style.colorBackground());
}

void GraphWidget::drawGrid(QPainter *painter, const QRect &rect, int channel, const DrawGraphOptions &options)
{
	Q_UNUSED(channel)
	//const Channel &ch = m_channels[channel];
	drawRectText(painter, rect, "grid", options.font(), options.channelOptions[channel].style.colorGrid());
}

void GraphWidget::drawYAxis(QPainter *painter, const QRect &rect, int channel, const DrawGraphOptions &options)
{
	Q_UNUSED(channel)
	//const Channel &ch = m_channels[channel];
	drawRectText(painter, rect, "y-axis", options.font(), options.channelOptions[channel].style.colorYAxis());
}

std::function<QPoint (const Sample &)> GraphWidget::createSampleToPointFn(GraphWidget::timemsec_t t1, GraphWidget::timemsec_t t2, double d1, double d2, const QRect &dest_rect)
{
	int le = dest_rect.left();
	int ri = dest_rect.right();
	int to = dest_rect.top();
	int bo = dest_rect.bottom();
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

std::function<Sample (const QPoint &)> GraphWidget::createPointToSampleFn(const QRect &src_rect, GraphWidget::timemsec_t t1, GraphWidget::timemsec_t t2, double d1, double d2)
{
	int le = src_rect.left();
	int ri = src_rect.right();
	int to = src_rect.top();
	int bo = src_rect.bottom();
	if(ri - le == 0)
		return nullptr;
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

void GraphWidget::drawSamples(QPainter *painter, const QRect &rect, int channel, const DrawGraphOptions &options)
{
	//shvLogFuncFrame() << "channel:" << channel;
	const Channel &ch = channelAt(channel);
	QPair<timemsec_t, timemsec_t> xrange = xRangeZoom();
	QPair<double, double> yrange = ch.yRangeZoom();

	timemsec_t t1 = xrange.first;
	timemsec_t t2 = xrange.second;
	double d1 = yrange.first;
	double d2 = yrange.second;
	auto sample2point = createSampleToPointFn(t1, t2, d1, d2, rect);

	if(!sample2point)
		return;

	const DrawChannelOptions &ch_opts = options.channelOptions[channel];
	int interpolation = ch_opts.style.interpolation();
	painter->save();
	QPen pen;
	pen.setColor(ch_opts.style.color());
	{
		double d = ch_opts.style.lineWidth();
		if(d > 0)
			pen.setWidth(options.unitToPixels(d));
		else
			pen.setWidth(2);
	}
	painter->setPen(pen);
	QColor line_area_color;
	if(ch_opts.style.lineAreaStyle() == ChannelStyle::LineAreaStyle::Filled) {
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

void GraphWidget::paintEvent(QPaintEvent *event)
{
	shvLogFuncFrame();
	Super::paintEvent(event);
	QPainter painter(this);
	draw(&painter, m_drawOptions.layout.rect, m_drawOptions);
}

void GraphWidget::mouseMoveEvent(QMouseEvent *event)
{
	//shvDebug() << event->pos().x() << event->pos().y();
	Super::mouseMoveEvent(event);
	QPoint vpos = event->pos();
	if(m_drawOptions.layout.miniMapRect.contains(vpos)) {
		shvDebug() << (vpos.x() - m_drawOptions.layout.miniMapRect.left()) << (vpos.y() - m_drawOptions.layout.miniMapRect.top());
	}
	else {
		for (int i = 0; i < m_channels.count(); ++i) {
			const Channel &ch = m_channels.at(i);
			const DrawChannelOptions &ch_opts = m_drawOptions.channelOptions[i];
			if(ch_opts.layout.graphRect.contains(vpos)) {
				std::function<Sample (const QPoint &)> point2sample = createPointToSampleFn(ch_opts.layout.graphRect, state.xRangeZoomMin, state.xRangeZoomMax, ch.state.yRangeZoomMin, ch.state.yRangeZoomMax);
				Sample s = point2sample(vpos);
				shvDebug() << (vpos.x() - m_drawOptions.layout.miniMapRect.left()) << (vpos.y() - m_drawOptions.layout.miniMapRect.top());
				shvDebug() << "time:" << s.time << "value:" << s.value.toDouble();
				break;
			}
		}
	}
}

} // namespace timeline
