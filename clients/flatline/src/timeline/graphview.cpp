#include "graphview.h"
#include "graphmodel.h"
#include "graphwidget.h"

#include <shv/core/exception.h>
#include <shv/coreqt/log.h>

#include <QPainter>
#include <QFontMetrics>
#include <QLabel>

#include <cmath>

namespace timeline {

GraphView::GraphView(QWidget *parent)
	: Super(parent)
{
	//setBackgroundRole(QPalette::Dark);
	//setWidget(new GraphWidget(this));
	//init();
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
void GraphView::setModel(GraphModel *model)
{
	m_model = model;
}

GraphModel *GraphView::model()
{
	return m_model;
}

void GraphView::createChannelsFromModel()
{
	m_channels.clear();
	GraphModel *m = model();
	for (int i = 0; i < m->channelCount(); ++i) {
		m_channels.append(Channel());
	}
}

void GraphView::clearChannels()
{
	m_channels.clear();
}

void GraphView::appendChannel()
{
	m_channels.append(Channel());
}

GraphView::Channel &GraphView::channelAt(int ix)
{
	if(ix < 0 || ix >= m_channels.count())
		SHV_EXCEPTION("Index out of range.");
	return m_channels[ix];
}

const GraphView::Channel &GraphView::channelAt(int ix) const
{
	if(ix < 0 || ix >= m_channels.count())
		SHV_EXCEPTION("Index out of range.");
	return m_channels[ix];
}

QVariantMap GraphView::mergeMaps(const QVariantMap &base, const QVariantMap &overlay) const
{
	QVariantMap ret = base;
	QMapIterator<QString, QVariant> it(overlay);
	while(it.hasNext()) {
		it.next();
		ret[it.key()] = it.value();
	}
	return ret;
}

void GraphView::makeLayout()
{
	static int unit_size = 0;
	if(unit_size == 0) {
		QFontMetrics fm(font());
		unit_size = fm.lineSpacing();
	}
	makeLayout(unit_size, geometry().size() - QSize(2, 2));
}

void GraphView::makeLayout(int unit_size, const QSize &widget_size)
{
	shvLogFuncFrame() << widget();
	layout.unitSize = unit_size;
	auto u2px = [unit_size](double unit) -> int { return static_cast<int>(unit_size * unit); };
	QSize viewport_size;
	viewport_size.setWidth(widget_size.width());
	int grid_w = viewport_size.width();
	int x_axis_pos = 0;
	x_axis_pos += u2px(style.leftMargin());
	x_axis_pos += u2px(style.verticalHeaderWidth());
	x_axis_pos += u2px(style.yAxisWidth());
	grid_w -= x_axis_pos;
	grid_w -= u2px(style.rightMargin());
	layout.miniMapRect.setHeight(u2px(style.miniMapHeight()));
	layout.miniMapRect.setLeft(x_axis_pos);
	layout.miniMapRect.setWidth(grid_w);

	layout.xAxisRect = layout.miniMapRect;
	layout.xAxisRect.setHeight(u2px(style.xAxisHeight()));

	int sum_h_min = 0;
	struct Rest { int index; int rest; ChannelStyle style;};
	QVector<Rest> rests;
	for (int i = 0; i < m_channels.count(); ++i) {
		Channel &ch = m_channels[i];
		ChannelStyle ch_style = mergeMaps(channelStyle, ch.style);
		int ch_h = u2px(ch_style.heightMin());
		rests << Rest{i, u2px(ch_style.heightRange()), ch_style};
		ch.layout.graphRect.setLeft(x_axis_pos);
		ch.layout.graphRect.setWidth(grid_w);
		ch.layout.graphRect.setHeight(ch_h);
		sum_h_min += ch_h;
		if(i > 0)
			sum_h_min += u2px(style.channelSpacing());
	}
	sum_h_min += u2px(style.xAxisHeight());
	sum_h_min += u2px(style.miniMapHeight());
	int h_rest = widget_size.height();
	h_rest -= sum_h_min;
	h_rest -= u2px(style.topMargin());
	h_rest -= u2px(style.bottomMargin());
	if(h_rest > 0) {
		// distribute free widget height space to channel's rects heights
		std::sort(rests.begin(), rests.end(), [](const Rest &a, const Rest &b) {
			return a.rest < b.rest;
		});
		for (int i = 0; i < rests.count(); ++i) {
			int fair_rest = h_rest / (rests.count() - i);
			const Rest &r = rests[i];
			Channel &ch = m_channels[r.index];
			int h = u2px(r.style.heightRange());
			if(h > fair_rest)
				h = fair_rest;
			ch.layout.graphRect.setHeight(ch.layout.graphRect.height() + h);
			h_rest -= h;
		}
	}
	// shift channel rects
	int widget_height = 0;
	widget_height += u2px(style.topMargin());
	for (int i = m_channels.count() - 1; i >= 0; --i) {
		Channel &ch = m_channels[i];

		ch.layout.graphRect.moveTop(widget_height);
		//ch.layout.graphRect.setWidth(layout.navigationBarRect.width());

		ch.layout.verticalHeaderRect = ch.layout.graphRect;
		ch.layout.verticalHeaderRect.setX(u2px(style.leftMargin()));
		ch.layout.verticalHeaderRect.setWidth(u2px(style.verticalHeaderWidth()));

		ch.layout.yAxisRect = ch.layout.verticalHeaderRect;
		ch.layout.yAxisRect.moveLeft(ch.layout.verticalHeaderRect.right());
		ch.layout.yAxisRect.setWidth(u2px(style.yAxisWidth()));

		widget_height += ch.layout.graphRect.height();
		if(i > 0)
			widget_height += u2px(style.channelSpacing());
	}
	layout.xAxisRect.moveTop(widget_height);
	widget_height += u2px(style.xAxisHeight());
	layout.miniMapRect.moveTop(widget_height);
	widget_height += u2px(style.miniMapHeight());
	widget_height += u2px(style.bottomMargin());

	viewport_size.setHeight(widget_height);
	shvDebug() << "\tw:" << viewport_size.width() << "h:" << viewport_size.height();
	widget()->setMinimumSize(viewport_size);
	widget()->setMaximumSize(viewport_size);
	viewport()->update();
}

void GraphView::drawRectText(QPainter *painter, const QRect &rect, const QString &text, const QFont &font, const QColor &color, const QColor &background)
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

void GraphView::draw(QPainter *painter, const GraphDrawOptions &gr_options)
{
	shvLogFuncFrame();
	drawBackground(painter, gr_options);
	for (int i = 0; i < m_channels.count(); ++i) {
		const Channel &ch = m_channels[i];
		ChannelStyle ch_style = mergeMaps(channelStyle, ch.style);
		DrawChannelOptions ch_options{gr_options, ch.layout.graphRect, ch_style, style};
		drawVerticalHeader(painter, i, ch_options);
		drawYAxis(painter, i, ch_options);
		drawBackground(painter, i, ch_options);
		drawGrid(painter, i, ch_options);
		drawSamples(painter, i, ch_options);
	}
	drawMiniMap(painter, gr_options);
	drawXAxis(painter, gr_options);
}

void GraphView::drawBackground(QPainter *painter, const GraphDrawOptions &options)
{
	painter->fillRect(QRect{QPoint(), widget()->geometry().size()}, style.colorBackground());
	//painter->fillRect(QRect{QPoint(), widget()->geometry().size()}, Qt::blue);
}

void GraphView::drawMiniMap(QPainter *painter, const GraphDrawOptions &gr_options)
{
	//drawRectText(painter, layout.miniMapRect, "navigation bar", options.font, Qt::blue);
	//DrawChannelOptions ch_options{gr_options, layout.miniMapRect, ch_style, style};
	for (int i = 0; i < m_channels.count(); ++i) {
		const Channel &ch = m_channels[i];
		ChannelStyle ch_style = mergeMaps(channelStyle, ch.style);
		DrawChannelOptions ch_options{gr_options, layout.miniMapRect, ch_style, style};
		//drawBackground(painter, i, ch_options);
		drawSamples(painter, i, ch_options);
	}
}

void GraphView::drawXAxis(QPainter *painter, const GraphDrawOptions &options)
{
	drawRectText(painter, layout.xAxisRect, "x-axis", options.font, Qt::green);
}

void GraphView::drawVerticalHeader(QPainter *painter, int channel, const GraphView::DrawChannelOptions &options)
{
	const Channel &ch = m_channels[channel];
	QColor c = options.channelStyle.color();
	QColor bc = c;
	bc.setAlphaF(0.05);
	QString text = model()->channelData(channel, GraphModel::ChannelDataRole::Name).toString();
	if(text.isEmpty())
		text = "<no name>";
	QFont font = options.graphOptions.font;
	font.setBold(true);
	drawRectText(painter, ch.layout.verticalHeaderRect, text, font, c, bc);
}

void GraphView::drawBackground(QPainter *painter, int channel, const GraphView::DrawChannelOptions &options)
{
	//const Channel &ch = m_channels[channel];
	painter->fillRect(options.rect, options.channelStyle.colorBackground());
}

void GraphView::drawGrid(QPainter *painter, int channel, const GraphView::DrawChannelOptions &options)
{
	//const Channel &ch = m_channels[channel];
	drawRectText(painter, options.rect, "grid", options.graphOptions.font, options.channelStyle.colorGrid());
}

void GraphView::drawYAxis(QPainter *painter, int channel, const GraphView::DrawChannelOptions &options)
{
	//const Channel &ch = m_channels[channel];
	drawRectText(painter, options.rect, "y-axis", options.graphOptions.font, options.channelStyle.colorYAxis());
}

void GraphView::drawSamples(QPainter *painter, int channel, const GraphView::DrawChannelOptions &options)
{
	shvLogFuncFrame() << "channel:" << channel;
	const Channel &ch = channelAt(channel);
	QPair<timemsec_t, timemsec_t> xrange = xRange();
	QPair<double, double> yrange = ch.yRange();
	int le = options.rect.left();
	int ri = options.rect.right();
	int to = options.rect.top();
	int bo = options.rect.bottom();
	timemsec_t t1 = xrange.first;
	timemsec_t t2 = xrange.second;
	double d1 = yrange.first;
	double d2 = yrange.second;
	//shvDebug() << t1 << t2;
	if(t2 - t1 == 0)
		return;
	double kx = static_cast<double>(ri - le) / (t2 - t1);
	//shvDebug() << d1 << d2;
	if(std::abs(d2 - d1) < 1e-6)
		return;
	double ky = (to - bo) / (d2 - d1);

	auto sample2point = [le, bo, kx, t1, d1, ky](const GraphModel::Sample &s) -> QPoint {
		const timemsec_t t = s.time;
		const double d = GraphModel::valueToDouble(s.value);
		double x = le + (t - t1) * kx;
		double y = bo + (d - d1) * ky;
		//shvDebug() << t << d << "->" << x << y;
		return QPoint{static_cast<int>(x), static_cast<int>(y)};
	};

	int interpolation = options.channelStyle.interpolation();
	painter->save();
	QPen pen;
	pen.setColor(options.channelStyle.color());
	{
		double d = options.channelStyle.lineWidth();
		if(d > 0)
			pen.setWidth(options.graphOptions.unitToPixels(d));
		else
			pen.setWidth(2);
	}
	painter->setPen(pen);
	GraphModel *m = model();
	QPoint p1;
	shvDebug() << "count:" << m->count(channel);
	for (int i = 0; i < m->count(channel); ++i) {
		const GraphModel::Sample s = m->sampleAt(channel, i);
		const QPoint p2 = sample2point(s);
		if(i == 0) {
			p1 = p2;
		}
		else {
			if(p2.x() != p1.x()) {
				if(interpolation == Interpolation::Stepped) {
					QPoint p3{p2.x(), p1.y()};
					painter->drawLine(p1, p3);
					painter->drawLine(p3, p2);
				}
				else {
					painter->drawLine(p1, p2);
				}
				p1 = p2;
			}
		}
	}
	painter->restore();
}

void GraphView::resizeEvent(QResizeEvent *event)
{
	Super::resizeEvent(event);
	makeLayout();
}

void GraphView::paintEvent(QPaintEvent *event)
{
	shvLogFuncFrame();
	Super::paintEvent(event);
	QPainter painter(viewport());
	GraphDrawOptions options{font(), layout.unitSize};
	draw(&painter, options);
}

} // namespace timeline
