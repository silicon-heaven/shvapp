#include "graphwidget.h"
#include "graphmodel.h"
#include "graphwidget.h"

#include <shv/core/exception.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/rpcvalue.h>

#include <QPainter>
#include <QFontMetrics>
#include <QLabel>
#include <QMouseEvent>

#include <cmath>

namespace cp = shv::chainpack;

namespace timeline {

//==========================================
// Graph::Channel
//==========================================

//==========================================
// Graph::GraphStyle
//==========================================
void Graph::GraphStyle::init(QWidget *widget)
{
	QFont f = widget->font();
	setFont(f);
	QFontMetrics fm(f, widget);
	setUnitSize(fm.lineSpacing());
}

int Graph::Channel::valueToPos(double val) const
{
	auto val2pos = Graph::valueToPosFn(yRangeZoom(), WidgetRange{m_layout.yAxisRect.bottom(), m_layout.yAxisRect.top()});
	return val2pos? val2pos(val): 0;
}

double Graph::Channel::posToValue(int y) const
{
	auto pos2val = Graph::posToValueFn(WidgetRange{m_layout.yAxisRect.bottom(), m_layout.yAxisRect.top()}, yRangeZoom());
	return pos2val? pos2val(y): 0;
}

//==========================================
// Graph
//==========================================
Graph::Graph(QObject *parent)
	: QObject(parent)
{
}

void Graph::setModel(GraphModel *model)
{
	if(m_model)
		m_model->disconnect(this);
	m_model = model;
	//connect(m_model, &GraphModel::xRangeChanged, this, &Graph::onModelXRangeChanged);
}

GraphModel *Graph::model() const
{
	return m_model;
}

void Graph::createChannelsFromModel()
{
	static QVector<QColor> colors {
		Qt::magenta,
		Qt::cyan,
		Qt::blue,
		Qt::red,
		QColor("orange"),
	};
	m_channels.clear();
	if(!m_model)
		return;
	setXRange(m_model->xRange());
	for (int i = 0; i < m_model->channelCount(); ++i) {
		Channel &ch = appendChannel();
		ChannelStyle style = ch.style();
		style.setColor(colors.value(i % colors.count()));
		ch.setStyle(style);
		ch.setMetaTypeId(m_model->guessMetaType(i));
		YRange yrange = m_model->yRange(i);
		if(yrange.isEmpty())
			yrange = YRange{0, 1};
		setYRange(i, yrange);
	}
}

void Graph::clearChannels()
{
	m_channels.clear();
}

timeline::Graph::Channel &Graph::appendChannel()
{
	m_channels.append(Channel());
	Channel &ch = m_channels.last();
	ch.setModelIndex(m_channels.count() - 1);
	return ch;
}

Graph::Channel &Graph::channelAt(int ix)
{
	/// seg fault is better than exception in this case
	//if(ix < 0 || ix >= m_channels.count())
	//	SHV_EXCEPTION("Index out of range.");
	return m_channels[ix];
}

const Graph::Channel &Graph::channelAt(int ix) const
{
	/// seg fault is better than exception in this case
	//if(ix < 0 || ix >= m_channels.count())
	//	SHV_EXCEPTION("Index out of range.");
	return m_channels[ix];
}

Graph::DataRect Graph::dataRect(int channel_ix) const
{
	const Channel &ch = channelAt(channel_ix);
	return DataRect{xRangeZoom(), ch.yRangeZoom()};
}

timemsec_t Graph::miniMapPosToTime(int pos) const
{
	auto pos2time = posToTimeFn(QPoint{m_layout.miniMapRect.left(), m_layout.miniMapRect.right()}, xRange());
	return pos2time? pos2time(pos): 0;
}

int Graph::miniMapTimeToPos(timemsec_t time) const
{
	auto time2pos = timeToPosFn(xRange(), WidgetRange{m_layout.miniMapRect.left(), m_layout.miniMapRect.right()});
	return time2pos? time2pos(time): 0;
}

timemsec_t Graph::posToTime(int pos) const
{
	auto pos2time = posToTimeFn(QPoint{m_layout.xAxisRect.left(), m_layout.xAxisRect.right()}, xRangeZoom());
	return pos2time? pos2time(pos): 0;
}

int Graph::timeToPos(timemsec_t time) const
{
	auto time2pos = timeToPosFn(xRangeZoom(), WidgetRange{m_layout.xAxisRect.left(), m_layout.xAxisRect.right()});
	return time2pos? time2pos(time): 0;
}

Sample Graph::timeToSample(int channel_ix, timemsec_t time) const
{
	GraphModel *m = model();
	const Channel &ch = channelAt(channel_ix);
	int model_ix = ch.modelIndex();
	int ix1 = m->lessOrEqualIndex(model_ix, time);
	if(ix1 < 0)
		return Sample();
	int interpolation = ch.effectiveStyle.interpolation();
	//shvInfo() << channel_ix << "interpolation:" << interpolation;
	if(interpolation == ChannelStyle::Interpolation::None) {
		Sample s = m->sampleAt(model_ix, ix1);
		if(s.time == time)
			return s;
	}
	else if(interpolation == ChannelStyle::Interpolation::Stepped) {
		Sample s = m->sampleAt(model_ix, ix1);
		s.time = time;
		return s;
	}
	else if(interpolation == ChannelStyle::Interpolation::Line) {
		int ix2 = ix1 + 1;
		if(ix2 >= m->count(model_ix))
			return Sample();
		Sample s1 = m->sampleAt(model_ix, ix1);
		Sample s2 = m->sampleAt(model_ix, ix2);
		if(s1.time == s2.time)
			return Sample();
		double d = s1.value.toDouble() + (time - s1.time) * (s2.value.toDouble() - s1.value.toDouble()) / (s2.time - s1.time);
		//shvInfo() << time << d << "---" << s1.value << s2.value;
		return Sample(time, d);
	}
	return Sample();
}

Sample Graph::posToData(const QPoint &pos) const
{
	int ch_ix = posToChannel(pos);
	if(ch_ix < 0)
		return Sample();
	const Channel &ch = channelAt(ch_ix);
	auto point2data = pointToDataFn(ch.graphRect(), DataRect{xRangeZoom(), ch.yRangeZoom()});
	return point2data? point2data(pos): Sample();
}

QPoint Graph::dataToPos(int ch_ix, const Sample &s) const
{
	const Channel &ch = channelAt(ch_ix);
	auto data2point = dataToPointFn(DataRect{xRangeZoom(), ch.yRangeZoom()}, ch.graphRect());
	return data2point? data2point(s): QPoint();
}

void Graph::setCrossBarPos(const QPoint &pos)
{
	m_state.crossBarPos = pos;
}

int Graph::posToChannel(const QPoint &pos) const
{
	for (int i = 0; i < channelCount(); ++i) {
		const Channel &ch = channelAt(i);
		if(ch.graphRect().contains(pos)) {
			return i;
		}
	}
	return -1;
}

void Graph::setXRange(const XRange &r, bool keep_zoom)
{
	const auto old_r = m_state.xRange;
	m_state.xRange = r;
	if(!m_state.xRangeZoom.isValid() || !keep_zoom) {
		m_state.xRangeZoom = m_state.xRange;
	}
	else {
		if(m_state.xRangeZoom.max == old_r.max) {
			const auto old_zr = m_state.xRangeZoom;
			m_state.xRangeZoom = m_state.xRange;
			m_state.xRangeZoom.min = m_state.xRangeZoom.max - old_zr.interval();
		}
	}
	sanityXRangeZoom();
	makeXAxis();
	m_miniMapCache = QPixmap();
	emit presentationDirty(rect());
}

void Graph::setXRangeZoom(const XRange &r)
{
	XRange prev_r = m_state.xRangeZoom;
	XRange new_r = r;
	if(new_r.min < m_state.xRange.min)
		new_r.min = m_state.xRange.min;
	if(new_r.max > m_state.xRange.max)
		new_r.max = m_state.xRange.max;
	if(prev_r.max < new_r.max && prev_r.interval() == new_r.interval()) {
		/// if zoom is just moved right (not scaled), snap to xrange max
		const auto diff = m_state.xRange.max - new_r.max;
		if(diff < m_state.xRange.interval() / 20) {
			new_r.min += diff;
			new_r.max += diff;
		}
		//shvInfo() << "diff:" << (m_state.xRange.max - new_r.max) << (m_state.xRange.interval() / 20);
	}
	m_state.xRangeZoom = new_r;
	makeXAxis();
}

void Graph::setYRange(int channel_ix, const YRange &r)
{
	Channel &ch = m_channels[channel_ix];
	ch.m_state.yRange = r;
	resetZoom(channel_ix);
	makeYAxis(channel_ix);
}

void Graph::enlargeYRange(int channel_ix, double step)
{
	Channel &ch = m_channels[channel_ix];
	YRange r = ch.m_state.yRange;
	r.min -= step;
	r.max += step;
	setYRange(channel_ix, r);
}

void Graph::setYRangeZoom(int channel_ix, const YRange &r)
{
	Channel &ch = m_channels[channel_ix];
	ch.m_state.yRangeZoom = r;
	if(ch.m_state.yRangeZoom.min < ch.m_state.yRange.min)
		ch.m_state.yRangeZoom.min = ch.m_state.yRange.min;
	if(ch.m_state.yRangeZoom.max > ch.m_state.yRange.max)
		ch.m_state.yRangeZoom.max = ch.m_state.yRange.max;
	makeYAxis(channel_ix);
}

void Graph::resetZoom(int channel_ix)
{
	Channel &ch = m_channels[channel_ix];
	setYRangeZoom(channel_ix, ch.yRange());
	makeYAxis(channel_ix);
}

void Graph::sanityXRangeZoom()
{
	if(m_state.xRangeZoom.min < m_state.xRange.min)
		m_state.xRangeZoom.min = m_state.xRange.min;
	if(m_state.xRangeZoom.max > m_state.xRange.max)
		m_state.xRangeZoom.max = m_state.xRange.max;
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

void Graph::makeXAxis()
{
	shvLogFuncFrame();
	static constexpr int64_t MSec = 1;
	static constexpr int64_t Sec = 1000 * MSec;
	static constexpr int64_t Min = 60 * Sec;
	static constexpr int64_t Hour = 60 * Min;
	static constexpr int64_t Day = 24 * Hour;
	static constexpr int64_t Month = 30 * Day;
	static constexpr int64_t Year = 365 * Day;
	static const std::map<int64_t, XAxis> intervals
	{
		{1 * MSec, {0, 1, XAxis::LabelFormat::MSec}},
		{2 * MSec, {0, 2, XAxis::LabelFormat::MSec}},
		{5 * MSec, {0, 5, XAxis::LabelFormat::MSec}},
		{10 * MSec, {0, 5, XAxis::LabelFormat::MSec}},
		{20 * MSec, {0, 5, XAxis::LabelFormat::MSec}},
		{50 * MSec, {0, 5, XAxis::LabelFormat::MSec}},
		{100 * MSec, {0, 5, XAxis::LabelFormat::MSec}},
		{200 * MSec, {0, 5, XAxis::LabelFormat::MSec}},
		{500 * MSec, {0, 5, XAxis::LabelFormat::MSec}},
		{1 * Sec, {0, 1, XAxis::LabelFormat::Sec}},
		{2 * Sec, {0, 2, XAxis::LabelFormat::Sec}},
		{5 * Sec, {0, 5, XAxis::LabelFormat::Sec}},
		{10 * Sec, {0, 5, XAxis::LabelFormat::Sec}},
		{20 * Sec, {0, 5, XAxis::LabelFormat::Sec}},
		{30 * Sec, {0, 3, XAxis::LabelFormat::Sec}},
		{1 * Min, {0, 1, XAxis::LabelFormat::Min}},
		{2 * Min, {0, 2, XAxis::LabelFormat::Min}},
		{5 * Min, {0, 5, XAxis::LabelFormat::Min}},
		{10 * Min, {0, 5, XAxis::LabelFormat::Min}},
		{20 * Min, {0, 5, XAxis::LabelFormat::Min}},
		{30 * Min, {0, 3, XAxis::LabelFormat::Min}},
		{1 * Hour, {0, 1, XAxis::LabelFormat::Hour}},
		{2 * Hour, {0, 2, XAxis::LabelFormat::Hour}},
		{3 * Hour, {0, 3, XAxis::LabelFormat::Hour}},
		{6 * Hour, {0, 6, XAxis::LabelFormat::Hour}},
		{12 * Hour, {0, 6, XAxis::LabelFormat::Hour}},
		{1 * Day, {0, 1, XAxis::LabelFormat::Day}},
		{2 * Day, {0, 2, XAxis::LabelFormat::Day}},
		{5 * Day, {0, 5, XAxis::LabelFormat::Day}},
		{10 * Day, {0, 5, XAxis::LabelFormat::Day}},
		{20 * Day, {0, 5, XAxis::LabelFormat::Day}},
		{1 * Month, {0, 1, XAxis::LabelFormat::Month}},
		{3 * Month, {0, 1, XAxis::LabelFormat::Month}},
		{6 * Month, {0, 1, XAxis::LabelFormat::Month}},
		{1 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{2 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{5 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{10 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{20 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{50 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{100 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{200 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{500 * Year, {0, 1, XAxis::LabelFormat::Year}},
		{1000 * Year, {0, 1, XAxis::LabelFormat::Year}},
	};
	//XRange range = xRangeZoom();
	//int x0 = m_layout.xAxisRect.left();
	int tick_units = 5;
	int tick_px = u2px(tick_units);
	timemsec_t t1 = posToTime(0);
	timemsec_t t2 = posToTime(tick_px);
	//timemsec_t label_tick_interval = posToTime(m_layout.xAxisRect.left() + u2px(label_tick_units)) - range.min;
	//int label_every = 5;
	int64_t interval = t2 - t1;
	auto lb = intervals.lower_bound(interval);
	if(lb == intervals.end())
		lb = --intervals.end();
	XAxis &axis = m_state.axis;
	axis = lb->second;
	axis.tickInterval = lb->first;
	shvDebug() << "interval:" << axis.tickInterval;
}

void Graph::makeYAxis(int channel)
{
	shvLogFuncFrame();
	Channel &ch = m_channels[channel];
	if(ch.yAxisRect().height() == 0)
		return;
	YRange range = ch.yRangeZoom();
	if(qFuzzyIsNull(range.interval()))
		return;
	int tick_units = 1;
	int tick_px = u2px(tick_units);
	double d1 = ch.posToValue(0);
	double d2 = ch.posToValue(tick_px);
	double interval = d1 - d2;
	if(qFuzzyIsNull(interval)) {
		shvError() << "channel:" << channel << "Y axis interval == 0";
		return;
	}
	shvDebug() << channel << "tick interval:" << interval << "range interval:" << range.interval();
	double pow = 1;
	if( interval >= 1 ) {
		while(interval >= 7) {
			interval /= 10;
			pow *= 10;
		}
	}
	else {
		while(interval > 0 && interval < 1) {
			interval *= 10;
			pow /= 10;
		}
	}

	Channel::YAxis &axis = ch.m_state.axis;
	// snap to closest 1, 2, 5
	if(interval < 1.5) {
		axis.tickInterval = 1 * pow;
		axis.subtickEvery = tick_units;
	}
	else if(interval < 3) {
		axis.tickInterval = 2 * pow;
		axis.subtickEvery = tick_units;
	}
	else if(interval < 7) {
		axis.tickInterval = 5 * pow;
		axis.subtickEvery = tick_units;
	}
	else if(interval < 10) {
		axis.tickInterval = 5 * pow * 10;
		axis.subtickEvery = tick_units;
	}
	else
		shvWarning() << "snapping interval error, interval:" << interval;
	//axis.tickInterval = axis.subtickEvery * pow;
	shvDebug() << channel << "axis.tickInterval:" << axis.tickInterval << "subtickEvery:" << axis.subtickEvery;
}

//void Graph::onModelXRangeChanged(const XRange &range)
//{
//	setXRange(range, true);
//}

int Graph::u2px(double u) const
{
	int sz = effectiveStyle.unitSize();
	return static_cast<int>(sz * u);
}

void Graph::makeLayout(const QRect &rect)
{
	m_miniMapCache = QPixmap();

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

	makeXAxis();
	for (int i = m_channels.count() - 1; i >= 0; --i) {
		makeYAxis(i);
	}
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

void Graph::draw(QPainter *painter, const QRect &dirty_rect)
{
	//shvLogFuncFrame();
	drawBackground(painter);
	for (int i = 0; i < m_channels.count(); ++i) {
		const Channel &ch = m_channels[i];
		if(dirty_rect.intersects(ch.graphRect())) {
			drawBackground(painter, i);
			drawGrid(painter, i);
			drawSamples(painter, i);
			drawCrossBar(painter, i);
		}
		if(dirty_rect.intersects(ch.verticalHeaderRect()))
			drawVerticalHeader(painter, i);
		if(dirty_rect.intersects(ch.yAxisRect()))
			drawYAxis(painter, i);
	}
	if(dirty_rect.intersects(m_layout.miniMapRect))
		drawMiniMap(painter);
	if(dirty_rect.intersects(m_layout.xAxisRect))
		drawXAxis(painter);
}

void Graph::drawBackground(QPainter *painter)
{
	painter->fillRect(m_layout.rect, effectiveStyle.colorBackground());
	//painter->fillRect(QRect{QPoint(), widget()->geometry().size()}, Qt::blue);
}

void Graph::drawMiniMap(QPainter *painter)
{
	if(m_miniMapCache.isNull()) {
		shvDebug() << "creating minimap cache";
		m_miniMapCache = QPixmap(m_layout.miniMapRect.width(), m_layout.miniMapRect.height());
		QRect mm_rect(QPoint(), m_layout.miniMapRect.size());
		QPainter p(&m_miniMapCache);
		QPainter *painter2 = &p;
		painter2->fillRect(mm_rect, m_channelStyle.colorBackground());
		for (int i = 0; i < m_channels.count(); ++i) {
			const Channel &ch = m_channels[i];
			ChannelStyle ch_st = ch.effectiveStyle;
			ch_st.setLineAreaStyle(ChannelStyle::LineAreaStyle::Filled);
			DataRect drect{xRange(), ch.yRange()};
			drawSamples(painter2, i, drect, mm_rect, ch_st);
		}

	}
	painter->drawPixmap(m_layout.miniMapRect.topLeft(), m_miniMapCache);
	int x1 = miniMapTimeToPos(xRangeZoom().min);
	int x2 = miniMapTimeToPos(xRangeZoom().max);
	painter->save();
	QPen pen;
	pen.setWidth(u2px(0.2));
	QPoint p1{x1, m_layout.miniMapRect.top()};
	QPoint p2{x1, m_layout.miniMapRect.bottom()};
	QPoint p3{x2, m_layout.miniMapRect.bottom()};
	QPoint p4{x2, m_layout.miniMapRect.top()};
	QColor bc(Qt::black);
	bc.setAlphaF(0.6);
	painter->fillRect(QRect{m_layout.miniMapRect.topLeft(), p2}, bc);
	painter->fillRect(QRect{p4, m_layout.miniMapRect.bottomRight()}, bc);
	pen.setColor(Qt::gray);
	painter->setPen(pen);
	painter->drawLine(m_layout.miniMapRect.topLeft(), p1);
	pen.setColor(Qt::white);
	painter->setPen(pen);
	painter->drawLine(p1, p2);
	painter->drawLine(p2, p3);
	painter->drawLine(p3, p4);
	pen.setColor(Qt::gray);
	painter->setPen(pen);
	painter->drawLine(p4, m_layout.miniMapRect.topRight());
	painter->restore();
}

void Graph::drawVerticalHeader(QPainter *painter, int channel)
{
	const Channel &ch = m_channels[channel];
	QColor c = effectiveStyle.color();
	QColor bc = c;
	bc.setAlphaF(0.05);
	QString name = model()->channelData(ch.modelIndex(), GraphModel::ChannelDataRole::Name).toString();
	if(name.isEmpty())
		name = "<no name>";
	QString shv_path = model()->channelData(ch.modelIndex(), GraphModel::ChannelDataRole::ShvPath).toString();
	if(shv_path.isEmpty())
		shv_path = "<no path>";
	//shvInfo() << channel << shv_path << name;
	QFont font = effectiveStyle.font();
	//drawRectText(painter, ch.m_layout.verticalHeaderRect, name, font, c, bc);
	painter->save();
	painter->fillRect(ch.m_layout.verticalHeaderRect, bc);
	QPen pen;
	pen.setColor(c);
	painter->setPen(pen);
	painter->drawRect(ch.m_layout.verticalHeaderRect);

	painter->setClipRect(ch.m_layout.verticalHeaderRect);

	font.setBold(true);
	QFontMetrics fm(font);
	painter->setFont(font);
	QRect text_rect = ch.m_layout.verticalHeaderRect;
	painter->drawText(text_rect, name);

	font.setBold(false);
	painter->setFont(font);
	text_rect.moveTop(text_rect.top() + fm.lineSpacing());
	painter->drawText(text_rect, shv_path);

	painter->restore();
}

void Graph::drawBackground(QPainter *painter, int channel)
{
	const Channel &ch = m_channels[channel];
	painter->fillRect(ch.m_layout.graphRect, ch.effectiveStyle.colorBackground());
}

void Graph::drawGrid(QPainter *painter, int channel)
{
	const Channel &ch = m_channels[channel];
	const XAxis &x_axis = m_state.axis;
	if(x_axis.tickInterval == 0) {
		drawRectText(painter, ch.m_layout.graphRect, "grid", effectiveStyle.font(), ch.effectiveStyle.colorGrid());
		return;
	}
	QColor gc = ch.effectiveStyle.colorGrid();
	if(!gc.isValid())
		return;
	painter->save();
	QPen pen_solid;
	pen_solid.setWidth(1);
	//pen.setWidth(u2px(0.1));
	pen_solid.setColor(gc);
	painter->setPen(pen_solid);
	painter->drawRect(ch.m_layout.graphRect);
	QPen pen_dot = pen_solid;
	pen_dot.setStyle(Qt::DotLine);
	painter->setPen(pen_dot);
	{
		// draw X-axis grid
		const XRange range = xRangeZoom();
		timemsec_t t1 = range.min / x_axis.tickInterval;
		t1 *= x_axis.tickInterval;
		if(t1 < range.min)
			t1 += x_axis.tickInterval;
		for (timemsec_t t = t1; t <= range.max; t += x_axis.tickInterval) {
			int x = timeToPos(t);
			QPoint p1{x, ch.graphRect().top()};
			QPoint p2{x, ch.graphRect().bottom()};
			painter->drawLine(p1, p2);
		}
	}
	{
		// draw Y-axis grid
		const YRange range = ch.yRangeZoom();
		const Channel::YAxis &y_axis = ch.m_state.axis;
		double d1 = std::ceil(range.min / y_axis.tickInterval);
		d1 *= y_axis.tickInterval;
		if(d1 < range.min)
			d1 += y_axis.tickInterval;
		for (double d = d1; d <= range.max; d += y_axis.tickInterval) {
			int y = ch.valueToPos(d);
			QPoint p1{ch.graphRect().left(), y};
			QPoint p2{ch.graphRect().right(), y};
			if(qFuzzyIsNull(d)) {
				painter->setPen(pen_solid);
				painter->drawLine(p1, p2);
				painter->setPen(pen_dot);
			}
			else {
				painter->drawLine(p1, p2);
			}
		}
	}
	painter->restore();
}

void Graph::drawXAxis(QPainter *painter)
{
	const XAxis &axis = m_state.axis;
	if(axis.tickInterval == 0) {
		drawRectText(painter, m_layout.xAxisRect, "x-axis", effectiveStyle.font(), Qt::green);
		return;
	}
	painter->save();
	QFont font = effectiveStyle.font();
	QFontMetrics fm(font);
	QPen pen;
	pen.setWidth(u2px(0.1));
	pen.setColor(effectiveStyle.colorAxis());
	int tick_len = u2px(0.15);
	painter->setPen(pen);
	painter->drawLine(m_layout.xAxisRect.topLeft(), m_layout.xAxisRect.topRight());

	const XRange range = xRangeZoom();
	if(axis.subtickEvery > 1) {
		timemsec_t subtick_interval = axis.tickInterval / axis.subtickEvery;
		timemsec_t t0 = range.min / subtick_interval;
		t0 *= subtick_interval;
		if(t0 < range.min)
			t0 += subtick_interval;
		for (timemsec_t t = t0; t <= range.max; t += subtick_interval) {
			int x = timeToPos(t);
			QPoint p1{x, m_layout.xAxisRect.top()};
			QPoint p2{p1.x(), p1.y() + tick_len};
			painter->drawLine(p1, p2);
		}
	}
	timemsec_t t1 = range.min / axis.tickInterval;
	t1 *= axis.tickInterval;
	if(t1 < range.min)
		t1 += axis.tickInterval;
	for (timemsec_t t = t1; t <= range.max; t += axis.tickInterval) {
		int x = timeToPos(t);
		QPoint p1{x, m_layout.xAxisRect.top()};
		QPoint p2{p1.x(), p1.y() + 2*tick_len};
		painter->drawLine(p1, p2);
		QString text;
		switch (axis.labelFormat) {
		case XAxis::LabelFormat::MSec:
			text = QStringLiteral("%1.%2").arg((t / 1000) % 1000).arg(t % 1000, 3, 10, QChar('0'));
			break;
		case XAxis::LabelFormat::Sec:
			text = QString::number((t / 1000) % 1000);
			break;
		case XAxis::LabelFormat::Min:
		case XAxis::LabelFormat::Hour: {
			QTime tm = QDateTime::fromMSecsSinceEpoch(t).time();
			text = QStringLiteral("%1:%2").arg(tm.hour()).arg(tm.minute(), 2, 10, QChar('0'));
			break;
		}
		case XAxis::LabelFormat::Day: {
			QDate dt = QDateTime::fromMSecsSinceEpoch(t).date();
			text = QStringLiteral("%1/%2").arg(dt.month()).arg(dt.day(), 2, 10, QChar('0'));
			break;
		}
		case XAxis::LabelFormat::Month: {
			QDate dt = QDateTime::fromMSecsSinceEpoch(t).date();
			text = QStringLiteral("%1-%2").arg(dt.year()).arg(dt.month(), 2, 10, QChar('0'));
			break;
		}
		case XAxis::LabelFormat::Year: {
			QDate dt = QDateTime::fromMSecsSinceEpoch(t).date();
			text = QStringLiteral("%1").arg(dt.year());
			break;
		}
		}
		QRect r = fm.boundingRect(text);
		r.moveTopLeft(p2 + QPoint{-r.width() / 2, 0});
		painter->drawText(r, text);
	}
	{
		QString text;
		switch (axis.labelFormat) {
		case XAxis::LabelFormat::MSec:
		case XAxis::LabelFormat::Sec:
			text = QStringLiteral("sec");
			break;
		case XAxis::LabelFormat::Min:
		case XAxis::LabelFormat::Hour: {
			text = QStringLiteral("hour:min");
			break;
		}
		case XAxis::LabelFormat::Day: {
			text = QStringLiteral("month/day");
			break;
		}
		case XAxis::LabelFormat::Month: {
			text = QStringLiteral("year-month");
			break;
		}
		case XAxis::LabelFormat::Year: {
			text = QStringLiteral("year");
			break;
		}
		}
		text = '[' + text + ']';
		QRect r = fm.boundingRect(text);
		r.moveTopLeft(m_layout.xAxisRect.topRight() + QPoint{-r.width() - u2px(0.2), 2*tick_len});
		painter->fillRect(r, effectiveStyle.colorBackground());
		painter->drawText(r, text);
	}
	painter->restore();
}

void Graph::drawYAxis(QPainter *painter, int channel)
{
	const Channel &ch = m_channels[channel];
	const Channel::YAxis &axis = ch.m_state.axis;
	if(qFuzzyCompare(axis.tickInterval, 0)) {
		drawRectText(painter, ch.m_layout.yAxisRect, "y-axis", effectiveStyle.font(), ch.effectiveStyle.colorAxis());
		return;
	}
	painter->save();
	QFont font = effectiveStyle.font();
	QFontMetrics fm(font);
	QPen pen;
	pen.setWidth(u2px(0.1));
	pen.setColor(ch.effectiveStyle.colorAxis());
	int tick_len = u2px(0.15);
	painter->setPen(pen);
	painter->drawLine(ch.m_layout.yAxisRect.bottomRight(), ch.m_layout.yAxisRect.topRight());

	const YRange range = ch.yRangeZoom();
	if(axis.subtickEvery > 1) {
		double subtick_interval = axis.tickInterval / axis.subtickEvery;
		double d0 = std::ceil(range.min / subtick_interval);
		d0 *= subtick_interval;
		if(d0 < range.min)
			d0 += subtick_interval;
		for (double d = d0; d <= range.max; d += subtick_interval) {
			int y = ch.valueToPos(d);
			QPoint p1{ch.m_layout.yAxisRect.right(), y};
			QPoint p2{p1.x() - tick_len, p1.y()};
			painter->drawLine(p1, p2);
		}
	}
	double d1 = std::ceil(range.min / axis.tickInterval);
	d1 *= axis.tickInterval;
	if(d1 < range.min)
		d1 += axis.tickInterval;
	for (double d = d1; d <= range.max; d += axis.tickInterval) {
		int y = ch.valueToPos(d);
		QPoint p1{ch.m_layout.yAxisRect.right(), y};
		QPoint p2{p1.x() - 2*tick_len, p1.y()};
		painter->drawLine(p1, p2);
		QString s = QString::number(d);
		QRect r = fm.boundingRect(s);
		painter->drawText(QPoint{p2.x() - u2px(0.2) - r.width(), p1.y() + r.height()/4}, s);
	}
	painter->restore();
}

std::function<QPoint (const Sample &)> Graph::dataToPointFn(const DataRect &src, const QRect &dest)
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
	//shvInfo() << "t1:" << t1 << "t2:" << t2 << "le:" << le << "ri:" << ri << "kx:" << kx;
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

std::function<Sample (const QPoint &)> Graph::pointToDataFn(const QRect &src, const DataRect &dest)
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

std::function<timemsec_t (int)> Graph::posToTimeFn(const QPoint &src, const XRange &dest)
{
	int le = src.x();
	int ri = src.y();
	if(ri - le == 0)
		return nullptr;
	timemsec_t t1 = dest.min;
	timemsec_t t2 = dest.max;
	return [t1, t2, le, ri](int x) {
		return static_cast<timemsec_t>(t1 + static_cast<double>(x - le) * (t2 - t1) / (ri - le));
	};
}

std::function<int (timemsec_t)> Graph::timeToPosFn(const XRange &src, const timeline::Graph::WidgetRange &dest)
{
	timemsec_t t1 = src.min;
	timemsec_t t2 = src.max;
	if(t2 - t1 <= 0)
		return nullptr;
	int le = dest.min;
	int ri = dest.max;
	return [t1, t2, le, ri](timemsec_t t) {
		return static_cast<timemsec_t>(le + static_cast<double>(t - t1) * (ri - le) / (t2 - t1));
	};
}

std::function<int (double)> Graph::valueToPosFn(const YRange &src, const Graph::WidgetRange &dest)
{
	double d1 = src.min;
	double d2 = src.max;
	if(std::abs(d2 - d1) < 1e-6)
		return nullptr;

	int y1 = dest.min;
	int y2 = dest.max;
	return [d1, d2, y1, y2](double val) {
		return static_cast<int>(y1 + static_cast<double>(val - d1) * (y2 - y1) / (d2 - d1));
	};
}

std::function<double (int)> Graph::posToValueFn(const Graph::WidgetRange &src, const YRange &dest)
{
	int bo = src.min;
	int to = src.max;
	if(bo == to)
		return nullptr;

	double d1 = dest.min;
	double d2 = dest.max;
	return [d1, d2, to, bo](int y) {
		return d1 + (y - bo) * (d2 - d1) / (to - bo);
	};
}

void Graph::drawSamples(QPainter *painter, int channel_ix, const DataRect &src_rect, const QRect &dest_rect, const Graph::ChannelStyle &channel_style)
{
	//shvLogFuncFrame() << "channel:" << channel_ix;
	const Channel &ch = channelAt(channel_ix);
	int model_ix = ch.modelIndex();
	QRect rect = dest_rect.isEmpty()? ch.m_layout.graphRect: dest_rect;
	ChannelStyle ch_style = channel_style.isEmpty()? ch.effectiveStyle: channel_style;

	XRange xrange;
	YRange yrange;
	if(!src_rect.isValid()) {
		xrange = xRangeZoom();
		yrange = ch.yRangeZoom();
	}
	else {
		xrange = src_rect.xRange;
		yrange = src_rect.yRange;
	}
	auto sample2point = dataToPointFn(DataRect{xrange, yrange}, rect);

	if(!sample2point)
		return;

	int interpolation = ch_style.interpolation();

	int sample_point_size = u2px(0.2);

	painter->save();
	painter->setClipRect(rect);
	QPen pen;
	QColor line_color = ch_style.color();
	pen.setColor(line_color);
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
	int ix1 = m->lessOrEqualIndex(model_ix, xrange.min);
	//ix1--; // draw one more sample to correctly display connection line to the first one in the zoom window
	if(ix1 < 0)
		ix1 = 0;
	int ix2 = m->lessOrEqualIndex(model_ix, xrange.max) + 1;
	ix2++; // draw one more sample to correctly display (n-1)th one
	//shvInfo() << channel << "range:" << xrange.min << xrange.max;
	//shvDebug() << "\t" << m->channelData(model_ix, GraphModel::ChannelDataRole::Name).toString()
	//		   << "from:" << ix1 << "to:" << ix2 << "cnt:" << (ix2 - ix1);
	constexpr int NO_X = std::numeric_limits<int>::min();
	struct OnePixelPoints {
		int x = NO_X;
		int lastY = 0;
		int minY = std::numeric_limits<int>::max();
		int maxY = std::numeric_limits<int>::min();
	};
	OnePixelPoints current_px, recent_px;
	int cnt = m->count(model_ix);
	for (int i = ix1; i <= ix2 && i <= cnt; ++i) {
		// sample is drawn one step behind, so one more loop is needed
		const Sample s = (i < cnt)? m->sampleAt(model_ix, i): Sample();
		const QPoint sample_point = sample2point(s);
		//shvDebug() << i << "t:" << s.time << "x:" << sample_point.x() << "y:" << sample_point.y();
		//shvDebug() << "\t recent x:" << recent_px.x << " current x:" << current_px.x;
		if(sample_point.x() == current_px.x) {
			current_px.minY = qMin(current_px.minY, sample_point.y());
			current_px.maxY = qMax(current_px.maxY, sample_point.y());
		}
		else {
			if(current_px.x != NO_X) {
				// draw recent point
				QPoint drawn_point(current_px.x, current_px.lastY);
				if(current_px.maxY != current_px.minY) {
					drawn_point = QPoint{current_px.x, (current_px.minY + current_px.maxY) / 2};
					painter->drawLine(current_px.x, current_px.minY, current_px.x, current_px.maxY);
				}
				if(interpolation == ChannelStyle::Interpolation::None) {
					if(line_area_color.isValid()) {
						QPoint p0 = sample2point(Sample{s.time, 0}); // <- this can be computed ahead
						p0.setX(drawn_point.x());
						painter->fillRect(QRect{drawn_point, p0}, line_area_color);
					}
					QRect r0{QPoint(), QSize{sample_point_size, sample_point_size}};
					r0.moveCenter(drawn_point);
					painter->fillRect(r0, line_color);
				}
				else if(interpolation == ChannelStyle::Interpolation::Stepped) {
					if(recent_px.x != NO_X) {
						QPoint pa{recent_px.x, recent_px.lastY};
						if(line_area_color.isValid()) {
							QPoint p0 = sample2point(Sample{s.time, 0}); // <- this can be computed ahead
							p0.setX(drawn_point.x());
							painter->fillRect(QRect{pa + QPoint{1, 0}, p0}, line_area_color);
						}
						QPoint pb{drawn_point.x(), recent_px.lastY};
						painter->drawLine(pa, pb);
						painter->drawLine(pb, drawn_point);
					}
				}
				else {
					if(recent_px.x != NO_X) {
						QPoint pa{recent_px.x, recent_px.lastY};
						if(line_area_color.isValid()) {
							QPoint p0 = sample2point(Sample{s.time, 0});
							p0.setX(drawn_point.x());
							QPainterPath pp;
							pp.moveTo(pa);
							pp.lineTo(drawn_point);
							pp.lineTo(p0);
							pp.lineTo(pa.x(), p0.y());
							pp.closeSubpath();
							painter->fillPath(pp, line_area_color);
						}
						painter->drawLine(pa, drawn_point);
					}
				}
				recent_px = current_px;
			}
			current_px.x = sample_point.x();
			current_px.minY = current_px.maxY = sample_point.y();
		}
		current_px.lastY = sample_point.y();
	}
	painter->restore();
}

void Graph::drawCrossBar(QPainter *painter, int channel_ix)
{
	if(m_state.crossBarPos.isNull())
		return;
	const Channel &ch = channelAt(channel_ix);
	if(ch.graphRect().left() > m_state.crossBarPos.x() || ch.graphRect().right() < m_state.crossBarPos.y())
		return;

	painter->save();
	{
		/// draw point on sample graph
		timemsec_t t = posToTime(m_state.crossBarPos.x());
		Sample s = timeToSample(channel_ix, t);
		if(s.value.isValid()) {
			QPoint p = dataToPos(channel_ix, s);
			//shvInfo() << s.time << s.value.toString() << "---" << p.x() << p.y();
			int d = u2px(0.3);
			QRect rect(0, 0, d, d);
			rect.moveCenter(p);
			//painter->fillRect(rect, ch.effectiveStyle.color());
			painter->fillRect(rect, effectiveStyle.colorCrossBar());
			rect.adjust(2, 2, -2, -2);
			painter->fillRect(rect, Qt::black);
		}
	}
	int d = u2px(0.4);
	QRect focus_rect;
	if(ch.graphRect().top() < m_state.crossBarPos.y() && ch.graphRect().bottom() > m_state.crossBarPos.y()) {
		focus_rect = QRect(0, 0, d, d);
		focus_rect.moveCenter(m_state.crossBarPos);
	}
	QPen pen_solid;
	pen_solid.setColor(effectiveStyle.colorCrossBar());
	QPen pen_dash = pen_solid;
	QColor c50 = pen_solid.color();
	c50.setAlphaF(0.5);
	pen_dash.setColor(c50);
	pen_dash.setStyle(Qt::DashLine);
	painter->setPen(pen_dash);
	{
		/// draw vertical line
		if(focus_rect.isNull()) {
			QPoint p1{m_state.crossBarPos.x(), ch.graphRect().top()};
			QPoint p2{m_state.crossBarPos.x(), ch.graphRect().bottom()};
			painter->drawLine(p1, p2);
		}
		else {
			QPoint p1{m_state.crossBarPos.x(), ch.graphRect().top()};
			QPoint p2{m_state.crossBarPos.x(), focus_rect.top()};
			QPoint p4{m_state.crossBarPos.x(), focus_rect.bottom()};
			QPoint p3{m_state.crossBarPos.x(), ch.graphRect().bottom()};
			painter->drawLine(p1, p2);
			painter->drawLine(p3, p4);
		}
	}
	if(!focus_rect.isNull()) {
		painter->setClipRect(ch.graphRect());
		/// draw horizontal line
		QPoint p1{ch.graphRect().left(), m_state.crossBarPos.y()};
		QPoint p2{focus_rect.left(), m_state.crossBarPos.y()};
		QPoint p3{focus_rect.right(), m_state.crossBarPos.y()};
		QPoint p4{ch.graphRect().right(), m_state.crossBarPos.y()};
		painter->drawLine(p1, p2);
		painter->drawLine(p3, p4);

		/// draw point
		painter->setPen(pen_solid);
		painter->drawRect(focus_rect);

		//timemsec_t t = posToTime(m_state.crossBarPos.x());
		Sample s = posToData(m_state.crossBarPos);
		//shvDebug() << "time:" << s.time << "value:" << s.value.toDouble();
		cp::RpcValue::DateTime dt = cp::RpcValue::DateTime::fromMSecsSinceEpoch(s.time);
		QString str = QStringLiteral("%1, %2").arg(QString::fromStdString(dt.toIsoString())).arg(s.value.toDouble());
		painter->drawText(m_state.crossBarPos + QPoint{focus_rect.width(), -focus_rect.height()}, str);

	}
	painter->restore();
}

} // namespace timeline
