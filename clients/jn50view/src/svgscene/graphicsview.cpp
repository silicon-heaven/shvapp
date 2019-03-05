#include "graphicsview.h"
#include "log.h"

#include <QMouseEvent>

namespace svgscene {

GraphicsView::GraphicsView(QWidget *parent)
	: Super(parent)
{

}

void GraphicsView::zoomToFit()
{
	if(scene()) {
		QRectF sr = scene()->sceneRect();
		fitInView(sr, Qt::KeepAspectRatio);
	}
}

void GraphicsView::zoom(double delta, const QPoint &mouse_pos)
{
	nLogFuncFrame() << "delta:" << delta << "center_pos:" << mouse_pos.x() << mouse_pos.y();
	double factor = delta / 100;
	factor = 1 + factor;
	if(factor < 0)
		factor = 0.1;
	scale(factor, factor);

	QRect view_rect = QRect(viewport()->pos(), viewport()->size());
	QPoint view_center = view_rect.center();
 	QSize view_d(view_center.x() - mouse_pos.x(), view_center.y() - mouse_pos.y());
	view_d /= factor;
	view_center = QPoint(mouse_pos.x() + view_d.width(), mouse_pos.y() + view_d.height());
	QPointF new_scene_center = mapToScene(view_center);
	centerOn(new_scene_center);
}

void GraphicsView::paintEvent(QPaintEvent *event)
{
	//QPainter p(viewport());
	//p.fillRect(sceneRect(), QBrush(QColor(0, 0 , 255, 120)));
	//p.setPen(Qt::blue);
	//QRectF r = scene()->sceneRect();

	//p.drawRect(r);
	Super::paintEvent(event);
}

void GraphicsView::wheelEvent(QWheelEvent *ev)
{
	if(ev->orientation() == Qt::Vertical) {
		if(ev->modifiers() == Qt::ControlModifier) {
			double delta = ev->angleDelta().y();
			zoom(delta / 10, ev->pos());
			ev->accept();
			return;
		}
	}
	Super::wheelEvent(ev);
}

void GraphicsView::mousePressEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ControlModifier) {
		m_dragMouseStartPos = ev->pos();
		setCursor(QCursor(Qt::ClosedHandCursor));
		ev->accept();
		return;
	}
	Super::mousePressEvent(ev);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent* ev)
{
	if (ev->button() == Qt::LeftButton && ev->modifiers() == Qt::ControlModifier) {
		setCursor(QCursor());
	}
	Super::mouseReleaseEvent(ev);
}

void GraphicsView::mouseMoveEvent(QMouseEvent* ev)
{
	if(ev->buttons() == Qt::LeftButton && ev->modifiers() == Qt::ControlModifier) {
		QPoint pos = ev->pos();
		QRect view_rect = QRect(viewport()->pos(), viewport()->size());
		QPoint view_center = view_rect.center();
		QPoint d(pos.x() - m_dragMouseStartPos.x(), pos.y() - m_dragMouseStartPos.y());
		view_center -= d;
		QPointF new_scene_center = mapToScene(view_center);
		centerOn(new_scene_center);
		m_dragMouseStartPos = pos;
		ev->accept();
		return;
	}
	Super::mouseMoveEvent(ev);
}

}
