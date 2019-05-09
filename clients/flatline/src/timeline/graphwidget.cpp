#include "graphwidget.h"
#include "graphmodel.h"
#include "graphwidget.h"

#include <shv/core/exception.h>
#include <shv/coreqt/log.h>
//#include <shv/chainpack/rpcvalue.h>

#include <QPainter>
#include <QFontMetrics>
#include <QLabel>
#include <QMouseEvent>
#include <QToolTip>
#include <QDateTime>

#include <cmath>

namespace cp = shv::chainpack;

namespace timeline {

GraphWidget::GraphWidget(QWidget *parent)
	: Super(parent)
{
	setMouseTracking(true);
}

void GraphWidget::setGraph(Graph *g)
{
	m_graph = g;
	Graph::GraphStyle style = graph()->style();
	style.init(this);
	graph()->setStyle(style);
}

Graph *GraphWidget::graph()
{
	return m_graph;
}

const Graph *GraphWidget::graph() const
{
	return m_graph;
}

void GraphWidget::makeLayout(const QRect &rect)
{
	shvLogFuncFrame();
	graph()->makeLayout(rect);
	setMinimumSize(graph()->rect().size());
	setMaximumSize(graph()->rect().size());
	update();
}

void GraphWidget::paintEvent(QPaintEvent *event)
{
	Super::paintEvent(event);
	QPainter painter(this);
	graph()->draw(&painter, event->rect());
}
/*
void GraphWidget::keyPressEvent(QKeyEvent *event)
{
	shvDebug() << event->key();
	//if(event->key() == Qt::)
	Super::keyPressEvent(event);
}

void GraphWidget::keyReleaseEvent(QKeyEvent *event)
{
	shvLogFuncFrame();
	Super::keyReleaseEvent(event);
}
*/
bool GraphWidget::isMouseAboveMiniMapHandle(const QPoint &mouse_pos, bool left) const
{
	const Graph *gr = graph();
	if(mouse_pos.y() < gr->miniMapRect().top() || mouse_pos.y() > gr->miniMapRect().bottom())
		return false;
	int x = left
			? gr->miniMapTimeToPos(gr->xRangeZoom().min)
			: gr->miniMapTimeToPos(gr->xRangeZoom().max);
	int w = gr->u2px(0.5);
	int x1 = left ? x - w : x - w/2;
	int x2 = left ? x + w/2 : x + w;
	return mouse_pos.x() > x1 && mouse_pos.x() < x2;
}

bool GraphWidget::isMouseAboveLeftMiniMapHandle(const QPoint &pos) const
{
	return isMouseAboveMiniMapHandle(pos, true);
}

bool GraphWidget::isMouseAboveRightMiniMapHandle(const QPoint &pos) const
{
	return isMouseAboveMiniMapHandle(pos, false);
}

bool GraphWidget::isMouseAboveMiniMapSlider(const QPoint &pos) const
{
	const Graph *gr = graph();
	if(pos.y() < gr->miniMapRect().top() || pos.y() > gr->miniMapRect().bottom())
		return false;
	int x1 = gr->miniMapTimeToPos(gr->xRangeZoom().min);
	int x2 = gr->miniMapTimeToPos(gr->xRangeZoom().max);
	return (x1 < pos.x()) && (pos.x() < x2);
}

int GraphWidget::isMouseAboveGraphArea(const QPoint &pos) const
{
	const Graph *gr = graph();
	int ch_ix = gr->posToChannel(pos);
	return ch_ix >= 0;
}

void GraphWidget::mousePressEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();
	if(event->button() == Qt::LeftButton) {
		if(isMouseAboveLeftMiniMapHandle(pos)) {
			m_mouseOperation = MouseOperation::MiniMapLeftResize;
			shvDebug() << "LEFT resize";
			event->accept();
			return;
		}
		else if(isMouseAboveRightMiniMapHandle(pos)) {
			m_mouseOperation = MouseOperation::MiniMapRightResize;
			event->accept();
			return;
		}
		else if(isMouseAboveMiniMapSlider(pos)) {
			m_mouseOperation = MouseOperation::MiniMapScrollZoom;
			m_recentMousePos = pos;
			event->accept();
			return;
		}
		else if(isMouseAboveGraphArea(pos) && event->modifiers() == Qt::ControlModifier) {
			m_mouseOperation = MouseOperation::GraphAreaMoveOrZoom;
			m_recentMousePos = pos;
			event->accept();
			return;
		}
	}
	Super::mousePressEvent(event);
}

void GraphWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if(event->button() == Qt::LeftButton) {
		m_mouseOperation = MouseOperation::None;
	}
	Super::mouseReleaseEvent(event);
}

void GraphWidget::mouseMoveEvent(QMouseEvent *event)
{
	//shvDebug() << event->pos().x() << event->pos().y();
	Super::mouseMoveEvent(event);
	QPoint pos = event->pos();
	Graph *gr = graph();
	if(isMouseAboveLeftMiniMapHandle(pos) || isMouseAboveRightMiniMapHandle(pos)) {
		//shvDebug() << (vpos.x() - gr->miniMapRect().left()) << (vpos.y() - gr->miniMapRect().top());
		setCursor(QCursor(Qt::SplitHCursor));
	}
	else if(isMouseAboveMiniMapSlider(pos)) {
		//shvDebug() << (vpos.x() - gr->miniMapRect().left()) << (vpos.y() - gr->miniMapRect().top());
		setCursor(QCursor(Qt::SizeHorCursor));
	}
	else {
		setCursor(QCursor(Qt::ArrowCursor));
	}
	switch (m_mouseOperation) {
	case MouseOperation::MiniMapLeftResize: {
		Graph::timemsec_t t = gr->miniMapPosToTime(pos.x());
		Graph::XRange r = gr->xRangeZoom();
		r.min = t;
		if(r.interval() > 0) {
			gr->setXRangeZoom(r);
			update();
		}
		return;
	}
	case MouseOperation::MiniMapRightResize: {
		Graph::timemsec_t t = gr->miniMapPosToTime(pos.x());
		Graph::XRange r = gr->xRangeZoom();
		r.max = t;
		if(r.interval() > 0) {
			gr->setXRangeZoom(r);
			update();
		}
		return;
	}
	case MouseOperation::MiniMapScrollZoom: {
		Graph::timemsec_t t2 = gr->miniMapPosToTime(pos.x());
		Graph::timemsec_t t1 = gr->miniMapPosToTime(m_recentMousePos.x());
		m_recentMousePos = pos;
		Graph::XRange r = gr->xRangeZoom();
		shvDebug() << "r.min:" << r.min << "r.max:" << r.max;
		Graph::timemsec_t dt = t2 - t1;
		shvDebug() << dt << "r.min:" << r.min << "-->" << (r.min + dt);
		r.min += dt;
		r.max += dt;
		if(r.min >= gr->xRange().min && r.max <= gr->xRange().max) {
			gr->setXRangeZoom(r);
			r = gr->xRangeZoom();
			shvDebug() << "new r.min:" << r.min << "r.max:" << r.max;
			update();
		}
		return;
	}
	case MouseOperation::GraphAreaMoveOrZoom: {
		Graph::timemsec_t t0 = gr->posToTime(m_recentMousePos.x());
		Graph::timemsec_t t1 = gr->posToTime(pos.x());
		Graph::timemsec_t dt = t0 - t1;
		Graph::XRange r = gr->xRangeZoom();
		r.min += dt;
		r.max += dt;
		gr->setXRangeZoom(r);
		m_recentMousePos = pos;
		update();
		return;
	}
	case MouseOperation::None:
		break;
	}
	int ch_ix = gr->posToChannel(pos);
	if(ch_ix >= 0) {
		setCursor(Qt::BlankCursor);
		gr->setCrossBarPos(pos);
		Graph::timemsec_t t = gr->posToTime(pos.x());
		Sample s = gr->timeToSample(ch_ix, t);
		if(s.isValid()) {
			const Graph::Channel &ch = gr->channelAt(ch_ix);
			shvDebug() << "time:" << s.time << "value:" << s.value.toDouble();
			QDateTime dt = QDateTime::fromMSecsSinceEpoch(s.time);
			//cp::RpcValue::DateTime dt = cp::RpcValue::DateTime::fromMSecsSinceEpoch(s.time);
			QString text = QStringLiteral("%1\n%2: %3")
					.arg(dt.toString(Qt::ISODateWithMs))
					.arg(gr->model()->channelData(ch.modelIndex(), timeline::GraphModel::ChannelDataRole::Name).toString())
					.arg(s.value.toDouble());
			QToolTip::showText(mapToGlobal(pos + QPoint{gr->u2px(0.8), 0}), text, this);
		}
		else {
			QToolTip::showText(QPoint(), QString());
		}
		update();
	}
	else {
		if(!gr->crossBarPos().isNull()) {
			gr->setCrossBarPos(QPoint());
			setCursor(Qt::ArrowCursor);
			QToolTip::showText(QPoint(), QString());
			update();
		}
	}
}

void GraphWidget::wheelEvent(QWheelEvent *event)
{
	bool is_zoom_on_slider = isMouseAboveMiniMapSlider(event->pos());
	bool is_zoom_on_graph = (event->modifiers() == Qt::ControlModifier) && isMouseAboveGraphArea(event->pos());
	constexpr int ZOOM_STEP = 10;
	if(is_zoom_on_slider) {
		Graph *gr = graph();
		double deg = event->angleDelta().y();
		//deg /= 120;
		// 120 deg ~ 1/20 of range
		Graph::timemsec_t dt = static_cast<Graph::timemsec_t>(deg * gr->xRangeZoom().interval() / 120 / ZOOM_STEP);
		Graph::XRange r = gr->xRangeZoom();
		r.min += dt;
		r.max -= dt;
		if(r.interval() > 1) {
			gr->setXRangeZoom(r);
			update();
		}
		event->accept();
		return;
	}
	if(is_zoom_on_graph) {
		Graph *gr = graph();
		double deg = event->angleDelta().y();
		//deg /= 120;
		// 120 deg ~ 1/20 of range
		Graph::timemsec_t dt = static_cast<Graph::timemsec_t>(deg * gr->xRangeZoom().interval() / 120 / ZOOM_STEP);
		Graph::XRange r = gr->xRangeZoom();
		double ratio = 0.5;
		// shift new zoom to center it horizontally on the mouse position
		Graph::timemsec_t t_mouse = gr->posToTime(event->pos().x());
		ratio = static_cast<double>(t_mouse - r.min) / r.interval();
		r.min += ratio * dt;
		r.max -= (1 - ratio) * dt;
		if(r.interval() > 1) {
			gr->setXRangeZoom(r);
			//r = gr->xRangeZoom();
			update();
		}
		event->accept();
		return;
	}
	Super::wheelEvent(event);
}

} // namespace timeline
