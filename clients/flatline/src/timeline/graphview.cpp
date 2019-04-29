#include "graphview.h"
#include "graphwidget.h"

namespace timeline {

GraphView::GraphView(QWidget *parent)
	: Super(parent)
{

}

void GraphView::makeLayout()
{
	if(GraphWidget *w = qobject_cast<GraphWidget*>(widget())) {
		QRect rect = {QPoint(), geometry().size() - QSize(2, 2)};
		w->makeLayout(rect);
	}
}

void GraphView::resizeEvent(QResizeEvent *event)
{
	Super::resizeEvent(event);
	makeLayout();
}

} // namespace timeline
