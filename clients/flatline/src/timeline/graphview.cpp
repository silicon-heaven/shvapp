#include "graphview.h"
#include "graphmodel.h"

#include <QPainter>

namespace timeline {

GraphView::GraphView(QWidget *parent)
	: Super(parent)
{

}

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
	QSize viewport_size;
	viewport_size.setWidth(geometry().width());
	int ch_w = viewport_size.width();
	int gr_left = 0;
	gr_left += u2px(m_options.style.leftMargin());
	gr_left += u2px(m_options.style.verticalHeaderWidth());
	gr_left += u2px(m_options.style.yAxisWidth());
	ch_w -= gr_left;
	ch_w -= u2px(m_options.style.rightMargin());
	m_state.navigationBarRect.setBottom(u2px(m_options.style.bottomMargin()));
	m_state.navigationBarRect.setHeight(u2px(m_options.style.navigationBarHeight()));
	m_state.navigationBarRect.setLeft(gr_left);
	m_state.navigationBarRect.setWidth(ch_w);

	m_state.xAxisRect = m_state.navigationBarRect;
	m_state.xAxisRect.moveBottom(m_state.navigationBarRect.top());
	m_state.xAxisRect.setHeight(u2px(m_options.style.xAxisHeight()));

	int sum_h_min = 0;
	struct Rest { int index; int rest; ChannelStyle style;};
	QVector<Rest> rests;
	for (int i = 0; i < m_channels.count(); ++i) {
		Channel &ch = m_channels[i];
		ChannelStyle style = mergeMaps(m_options.defaultChannelStyle, ch.style);
		int ch_h = u2px(style.heightMin());
		rests << Rest{i, u2px(style.heightRange()), style};
		ch.state.graphRect.setWidth(ch_w);
		ch.state.graphRect.setHeight(ch_h);
		sum_h_min += ch_h;
		if(i > 0)
			sum_h_min += u2px(m_options.style.channelSpacing());
	}
	sum_h_min += u2px(m_options.style.xAxisHeight());
	sum_h_min += u2px(m_options.style.navigationBarHeight());
	int h_rest = geometry().height();
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
			ch.state.graphRect.setHeight(ch.state.graphRect.height() + h);
			h_rest -= h;
		}
	}
	// shift channel rects
	int ch_rect_bottom = 0;
	ch_rect_bottom += u2px(m_options.style.bottomMargin());
	ch_rect_bottom += u2px(m_options.style.xAxisHeight());
	ch_rect_bottom += u2px(m_options.style.navigationBarHeight());
	for (int i = 0; i < m_channels.count(); ++i) {
		Channel &ch = m_channels[i];
		ch.state.graphRect.moveBottom(ch_rect_bottom);

		ch.state.verticalHeaderRect = ch.state.graphRect;
		ch.state.verticalHeaderRect.setX(u2px(m_options.style.leftMargin()));
		ch.state.verticalHeaderRect.setWidth(u2px(m_options.style.verticalHeaderWidth()));

		ch_rect_bottom += ch.state.graphRect.height();
		ch_rect_bottom += u2px(m_options.style.channelSpacing());
	}
	if(m_channels.count())
		ch_rect_bottom -= u2px(m_options.style.channelSpacing());
	viewport_size.setHeight(ch_rect_bottom);
	widget()->setGeometry(QRect({0, 0}, viewport_size));
}

void GraphView::paintEvent(QPaintEvent *event)
{
	Q_UNUSED(event)
	QPainter painter(viewport());
	draw(&painter);
}

void GraphView::draw(QPainter *painter)
{
	drawBackground(painter);
	drawNavigationBar(painter);
	drawXAxis(painter);
	for (int i = 0; i < m_channels.count(); ++i) {
		const Channel &ch = m_channels[i];
		ChannelStyle style = mergeMaps(m_options.defaultChannelStyle, ch.style);
		DrawChannelOptions options{ch.state.graphRect, style, m_options.style};
		drawBackground(painter, i, options);
		drawGrid(painter, i, options);
		drawYAxis(painter, i, options);
		drawSamples(painter, i, options);
	}
}

void GraphView::drawBackground(QPainter *painter)
{
	painter->fillRect(viewport()->geometry(), m_options.style.colorBackground());
}

void GraphView::drawNavigationBar(QPainter *painter)
{
	painter->fillRect(m_state.navigationBarRect, Qt::blue);
}

void GraphView::drawXAxis(QPainter *painter)
{
	painter->fillRect(m_state.xAxisRect, Qt::darkGreen);
}

void GraphView::drawVerticalHeader(QPainter *painter, int channel, const GraphView::DrawChannelOptions &options)
{
	const Channel &ch = m_channels[channel];
	painter->fillRect(ch.state.verticalHeaderRect, Qt::darkYellow);
}

void GraphView::drawBackground(QPainter *painter, int channel, const GraphView::DrawChannelOptions &options)
{
	const Channel &ch = m_channels[channel];
	painter->fillRect(ch.state.graphRect, options.channelStyle.colorBackground());
}

void GraphView::drawGrid(QPainter *painter, int channel, const GraphView::DrawChannelOptions &options)
{
	// NIY
}

void GraphView::drawYAxis(QPainter *painter, int channel, const GraphView::DrawChannelOptions &options)
{
	const Channel &ch = m_channels[channel];
	painter->fillRect(ch.state.yAxisRect, Qt::darkRed);
}

void GraphView::drawSamples(QPainter *painter, int channel, const GraphView::DrawChannelOptions &options)
{
	// NIY
}

} // namespace timeline
