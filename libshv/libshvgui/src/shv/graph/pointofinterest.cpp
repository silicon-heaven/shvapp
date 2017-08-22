#include "pointofinterest.h"
#include "graphview.h"

namespace shv {
namespace gui {
namespace graphview {

PointOfInterest::PointOfInterest(QObject *parent) : PointOfInterest(ValueChange::ValueX(), QString::null, QColor(), parent)
{
}

PointOfInterest::PointOfInterest(ValueChange::ValueX position, const QString &comment, const QColor &color, QObject *parent)
	: QObject(parent)
	, m_position(position)
	, m_comment(comment)
	, m_color(color)
{
	GraphView *graph = qobject_cast<GraphView*>(parent);
	if (graph) {
		graph->addPointOfInterest(this);
	}
}

}
}
}
