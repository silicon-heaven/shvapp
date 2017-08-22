#include "pointofinterest.h"
#include "view.h"

namespace shv {
namespace gui {
namespace graphview {

PointOfInterest::PointOfInterest(QObject *parent) : PointOfInterest(ValueChange(), QString::null, QColor(), Type::Vertical, 0, parent)
{
}

PointOfInterest::PointOfInterest(ValueChange::ValueX position, const QString &comment, const QColor &color, QObject *parent)
	: PointOfInterest(ValueChange(position, 0), comment, color, Type::Vertical, parent)
{
}

PointOfInterest::PointOfInterest(ValueChange position, const QString &comment, const QColor &color, PointOfInterest::Type poi_type, QObject *parent)
	: PointOfInterest(position, comment, color, poi_type, 0, parent)
{
}

PointOfInterest::PointOfInterest(ValueChange position, const QString &comment, const QColor &color, Type type, Serie *serie, QObject *parent)
	: QObject(parent)
	, m_position(position)
	, m_comment(comment)
	, m_color(color)
	, m_type(type)
	, m_serie(serie)
{
	GraphView *graph = qobject_cast<GraphView*>(parent);
	if (graph) {
		graph->addPointOfInterest(this);
	}
	Q_ASSERT_X(type == Type::Point || m_serie == 0, "Point of interest", "Vertical poi must not have related serie");
}

void PointOfInterest::setPosition(ValueChange position)
{
	m_position = position;
	update();
}

void PointOfInterest::setPosition(ValueChange::ValueX position)
{
	m_position = ValueChange(position, m_position.valueY);
	update();
}

void PointOfInterest::update()
{
	GraphView *graph = qobject_cast<GraphView*>(parent());
	if (graph) {
		graph->computeGeometry();
		graph->update();
	}
}

}
}
}
