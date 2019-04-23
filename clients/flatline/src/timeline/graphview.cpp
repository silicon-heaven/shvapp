#include "graphview.h"
#include "graphmodel.h"

namespace timeline {

GraphView::GraphView(QWidget *parent)
	: Super(parent)
{

}

void GraphView::setModel(GraphModel *model)
{
	m_model = model;
}

void GraphView::paintEvent(QPaintEvent *event)
{

}

} // namespace timeline
