#include "visuwidget.h"

#include <QPainter>
#include <QSvgRenderer>

VisuWidget::VisuWidget(QWidget *parent)
	: Super(parent)
	, m_renderer(new QSvgRenderer(this))
{
	//load(QStringLiteral(":/images/bfs1.svg"));
}

bool VisuWidget::load(const QByteArray &svg)
{
	bool ret = m_renderer->load(svg);
	if(ret)
		update();
	return ret;
}

void VisuWidget::paintEvent(QPaintEvent *event)
{
	Super::paintEvent(event);
	QPainter p(this);
	QSize svg_sz = m_renderer->defaultSize();
	QRect r = geometry();
	r.moveTopLeft({0, 0});
	p.fillRect(r, QColor("#2b4174"));
	QSize sz = r.size();
	sz = svg_sz.scaled(sz, Qt::KeepAspectRatio);
	r.setSize(sz);
	m_renderer->render(&p, r);
}
