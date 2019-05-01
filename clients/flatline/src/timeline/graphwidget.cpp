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
		for (int i = 0; i < gr->channelCount(); ++i) {
			const Graph::Channel &ch = gr->channelAt(i);
			if(ch.graphRect().contains(vpos)) {
				std::function<Sample (const QPoint &)> point2sample = Graph::createPointToSampleFn(ch.graphRect(), gr->dataRect(i));
				Sample s = point2sample(vpos);
				//shvDebug() << (vpos.x() - m_drawOptions.layout.miniMapRect.left()) << (vpos.y() - m_drawOptions.layout.miniMapRect.top());
				shvDebug() << "time:" << s.time << "value:" << s.value.toDouble();
				break;
			}
		}
	}
}

} // namespace timeline
