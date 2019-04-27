#include "graphwidget.h"

#include <shv/coreqt/log.h>

#include <QPainter>

namespace timeline {

GraphWidget::GraphWidget(QWidget *parent)
	: Super(parent)
	//, m_graphView(view)
{
	setStyleSheet("background:red");
}

void GraphWidget::paintEvent(QPaintEvent *event)
{
	shvLogFuncFrame();
	Super::paintEvent(event);
	//QPainter painter(this);
	//painter.fillRect(QRect{QPoint(), geometry().size()}, Qt::red);
	//DrawOptions options{font()};
	//draw(&painter, options);
}

} // namespace timeline
