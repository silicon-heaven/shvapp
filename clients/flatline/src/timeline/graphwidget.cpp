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
	setMouseTracking(true);
	Graph::GraphStyle style = graph()->style();
	style.init(this);
	graph()->setStyle(style);
}

void GraphWidget::setModel(GraphModel *m)
{
	m_graph.setModel(m);
}

Graph *GraphWidget::graph()
{
	return &m_graph;
}

const Graph *GraphWidget::graph() const
{
	return &m_graph;
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
	//shvLogFuncFrame();
	Super::paintEvent(event);
	QPainter painter(this);
	graph()->draw(&painter);
}

bool GraphWidget::isMouseAboveMiniMapHandle(const QPoint &mouse_pos, bool left) const
{
	const Graph *gr = graph();
	if(mouse_pos.y() < gr->miniMapRect().top() || mouse_pos.y() > gr->miniMapRect().bottom())
		return false;
	int x = left
			? gr->miniMapTimeToPos(gr->xRangeZoom().min)
			: gr->miniMapTimeToPos(gr->xRangeZoom().max);
	int w = gr->u2px(0.3);
	int x1 = left ? x - w : x + w / 2;
	int x2 = left ? x - w / 2 : x + w;
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

void GraphWidget::mousePressEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();
	if(event->button() == Qt::LeftButton) {
		if(isMouseAboveLeftMiniMapHandle(pos)) {
			m_miniMapOperation = MiniMapOperation::LeftResize;
			shvDebug() << "LEFT resize";
			return;
		}
		else if(isMouseAboveRightMiniMapHandle(pos)) {
			m_miniMapOperation = MiniMapOperation::RightResize;
			return;
		}
		else if(isMouseAboveMiniMapSlider(pos)) {
			m_miniMapOperation = MiniMapOperation::Scroll;
			m_miniMapScrollPos = pos.x();
			return;
		}
	}
	Super::mousePressEvent(event);
}

void GraphWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if(event->button() == Qt::LeftButton) {
		m_miniMapOperation = MiniMapOperation::None;
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
		int ch_ix = gr->setCrossBarPos(pos);
		if(ch_ix >= 0) {
			Graph::timemsec_t t = gr->posToTime(pos.x());
			Sample s = gr->timeToSample(ch_ix, t);
			shvDebug() << "time:" << s.time << "value:" << s.value.toDouble();
		}
	}
	switch (m_miniMapOperation) {
	case MiniMapOperation::LeftResize: {
		Graph::timemsec_t t = gr->miniMapPosToTime(pos.x());
		Graph::XRange r = gr->xRangeZoom();
		shvDebug() << r.min << "--->" << t;
		r.min = t;
		gr->setXRangeZoom(r);
		update();
		break;
	}
	case MiniMapOperation::RightResize: {
		Graph::timemsec_t t = gr->miniMapPosToTime(pos.x());
		Graph::XRange r = gr->xRangeZoom();
		r.max = t;
		gr->setXRangeZoom(r);
		update();
		break;
	}
	case MiniMapOperation::Scroll: {
		Graph::timemsec_t t2 = gr->miniMapPosToTime(pos.x());
		Graph::timemsec_t t1 = gr->miniMapPosToTime(m_miniMapScrollPos);
		m_miniMapScrollPos = pos.x();
		Graph::XRange r = gr->xRangeZoom();
		shvDebug() << "r.min:" << r.min << "r.max:" << r.max;
		Graph::timemsec_t dt = t2 - t1;
		shvDebug() << dt << "r.min:" << r.min << "-->" << (r.min + dt);
		r.min += dt;
		r.max += dt;
		gr->setXRangeZoom(r);
		r = gr->xRangeZoom();
		shvDebug() << "new r.min:" << r.min << "r.max:" << r.max;
		update();
		break;
	}
	case MiniMapOperation::None:
		break;
	}
}

} // namespace timeline
