#include "graphview.h"
#include "graphmodel.h"
#include "graphwidget.h"

#include <shv/core/exception.h>
#include <shv/coreqt/log.h>

#include <QPainter>
#include <QFontMetrics>
#include <QLabel>

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
	auto u2px = [unit_size](double unit) -> int { return static_cast<int>(unit_size * unit); };
	QSize viewport_size;
	viewport_size.setWidth(widget_size.width());
	int grid_w = viewport_size.width();
	int x_axis_pos = 0;
	x_axis_pos += u2px(m_options.style.leftMargin());
	x_axis_pos += u2px(m_options.style.verticalHeaderWidth());
	x_axis_pos += u2px(m_options.style.yAxisWidth());
	grid_w -= x_axis_pos;
	grid_w -= u2px(m_options.style.rightMargin());
	//layout.navigationBarRect.setBottom(u2px(m_options.style.bottomMargin()));
	layout.navigationBarRect.setHeight(u2px(m_options.style.navigationBarHeight()));
	layout.navigationBarRect.setLeft(x_axis_pos);
	layout.navigationBarRect.setWidth(grid_w);

	layout.xAxisRect = layout.navigationBarRect;
	//layout.xAxisRect.moveBottom(layout.navigationBarRect.top());
	layout.xAxisRect.setHeight(u2px(m_options.style.xAxisHeight()));

	int sum_h_min = 0;
	struct Rest { int index; int rest; ChannelStyle style;};
	QVector<Rest> rests;
	for (int i = 0; i < m_channels.count(); ++i) {
		Channel &ch = m_channels[i];
		ChannelStyle style = mergeMaps(m_options.defaultChannelStyle, ch.style);
		int ch_h = u2px(style.heightMin());
		rests << Rest{i, u2px(style.heightRange()), style};
		ch.layout.graphRect.setLeft(x_axis_pos);
		ch.layout.graphRect.setWidth(grid_w);
		ch.layout.graphRect.setHeight(ch_h);
		sum_h_min += ch_h;
		if(i > 0)
			sum_h_min += u2px(m_options.style.channelSpacing());
	}
	sum_h_min += u2px(m_options.style.xAxisHeight());
	sum_h_min += u2px(m_options.style.navigationBarHeight());
	int h_rest = widget_size.height();
	h_rest -= sum_h_min;
	h_rest -= u2px(m_options.style.topMargin());
	h_rest -= u2px(m_options.style.bottomMargin());
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
	widget_height += u2px(m_options.style.topMargin());
	for (int i = m_channels.count() - 1; i >= 0; --i) {
		Channel &ch = m_channels[i];

		ch.layout.graphRect.moveTop(widget_height);
		//ch.layout.graphRect.setWidth(layout.navigationBarRect.width());

		ch.layout.verticalHeaderRect = ch.layout.graphRect;
		ch.layout.verticalHeaderRect.setX(u2px(m_options.style.leftMargin()));
		ch.layout.verticalHeaderRect.setWidth(u2px(m_options.style.verticalHeaderWidth()));

		ch.layout.yAxisRect = ch.layout.verticalHeaderRect;
		ch.layout.yAxisRect.moveLeft(ch.layout.verticalHeaderRect.right());
		ch.layout.yAxisRect.setWidth(u2px(m_options.style.yAxisWidth()));

		widget_height += ch.layout.graphRect.height();
		if(i > 0)
			widget_height += u2px(m_options.style.channelSpacing());
	}
	layout.xAxisRect.moveTop(widget_height);
	widget_height += u2px(m_options.style.xAxisHeight());
	layout.navigationBarRect.moveTop(widget_height);
	widget_height += u2px(m_options.style.navigationBarHeight());
	widget_height += u2px(m_options.style.bottomMargin());

	viewport_size.setHeight(widget_height);
	shvDebug() << "\tw:" << viewport_size.width() << "h:" << viewport_size.height();
	widget()->setMinimumSize(viewport_size);
	widget()->setMaximumSize(viewport_size);
}

void GraphView::drawMockup(QPainter *painter, const QRect &rect, const QString &text, const QFont &font, const QColor &color)
{
	painter->save();
	//QColor bc = color.lighter();
	//painter->fillRect(rect, bc);
	QPen pen;
	pen.setColor(color);
	painter->setPen(pen);
	painter->drawRect(rect);
	painter->setFont(font);
	QFontMetrics fm(font);
	painter->drawText(rect.left() + fm.width('i'), rect.top() + fm.leading() + fm.ascent(), text);
	painter->restore();
}

void GraphView::draw(QPainter *painter, const DrawOptions &options)
{
	shvLogFuncFrame();
	drawBackground(painter, options);
	drawNavigationBar(painter, options);
	drawXAxis(painter, options);
	for (int i = 0; i < m_channels.count(); ++i) {
		const Channel &ch = m_channels[i];
		ChannelStyle style = mergeMaps(m_options.defaultChannelStyle, ch.style);
		DrawChannelOptions ch_options{options.font, ch.layout.graphRect, style, m_options.style};
		drawVerticalHeader(painter, i, ch_options);
		drawYAxis(painter, i, ch_options);
		drawBackground(painter, i, ch_options);
		drawGrid(painter, i, ch_options);
		drawSamples(painter, i, ch_options);
	}
}

void GraphView::drawBackground(QPainter *painter, const DrawOptions &options)
{
	painter->fillRect(QRect{QPoint(), widget()->geometry().size()}, m_options.style.colorBackground());
	//painter->fillRect(QRect{QPoint(), widget()->geometry().size()}, Qt::blue);
}

void GraphView::drawNavigationBar(QPainter *painter, const DrawOptions &options)
{
	drawMockup(painter, layout.navigationBarRect, "navigation bar", options.font, Qt::blue);
}

void GraphView::drawXAxis(QPainter *painter, const DrawOptions &options)
{
	drawMockup(painter, layout.xAxisRect, "x-axis", options.font, Qt::green);
}

void GraphView::drawVerticalHeader(QPainter *painter, int channel, const GraphView::DrawChannelOptions &options)
{
	const Channel &ch = m_channels[channel];
	drawMockup(painter, ch.layout.verticalHeaderRect, "header", options.font, Qt::yellow);
}

void GraphView::drawBackground(QPainter *painter, int channel, const GraphView::DrawChannelOptions &options)
{
	const Channel &ch = m_channels[channel];
	painter->fillRect(ch.layout.graphRect, options.channelStyle.colorBackground());
}

void GraphView::drawGrid(QPainter *painter, int channel, const GraphView::DrawChannelOptions &options)
{
	const Channel &ch = m_channels[channel];
	drawMockup(painter, ch.layout.graphRect, "grid", options.font, ch.style.colorGrid());
}

void GraphView::drawYAxis(QPainter *painter, int channel, const GraphView::DrawChannelOptions &options)
{
	const Channel &ch = m_channels[channel];
	drawMockup(painter, ch.layout.yAxisRect, "y-axis", options.font, ch.style.color());
}

void GraphView::drawSamples(QPainter *painter, int channel, const GraphView::DrawChannelOptions &options)
{
	// NIY
}

void GraphView::paintEvent(QPaintEvent *event)
{
	shvLogFuncFrame();
	Super::paintEvent(event);
	QPainter painter(viewport());
	DrawOptions options{font()};
	draw(&painter, options);
}

} // namespace timeline
