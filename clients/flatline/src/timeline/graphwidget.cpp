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
	shvLogFuncFrame();
	Super::paintEvent(event);
	QPainter painter(this);
	graph()->draw(&painter);
}

void GraphWidget::mouseMoveEvent(QMouseEvent *event)
{
	//shvDebug() << event->pos().x() << event->pos().y();
	Super::mouseMoveEvent(event);
	QPoint vpos = event->pos();
	Graph *gr = graph();
	if(gr->miniMapRect().contains(vpos)) {
		shvDebug() << (vpos.x() - gr->miniMapRect().left()) << (vpos.y() - gr->miniMapRect().top());
	}
	else {
		int ch_ix = gr->setCrossBarPos(vpos);
		if(ch_ix >= 0) {
			Graph::timemsec_t t = gr->posToTime(vpos);
			Sample s = gr->timeToSample(ch_ix, t);
			shvDebug() << "time:" << s.time << "value:" << s.value.toDouble();
		}
	}
}

} // namespace timeline
