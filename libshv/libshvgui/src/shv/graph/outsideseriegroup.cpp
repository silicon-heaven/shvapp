#include "outsideseriegroup.h"
#include "graphview.h"
#include "serie.h"

namespace shv {
namespace gui {
namespace graphview {

OutsideSerieGroup::OutsideSerieGroup(QObject *parent) : OutsideSerieGroup(QString::null, parent)
{
}

OutsideSerieGroup::OutsideSerieGroup(const QString &name, QObject *parent)
	: QObject(parent)
	, m_name(name)
{
	GraphView *graph = qobject_cast<GraphView*>(parent);
	if (graph) {
		graph->addOutsideSerieGroup(this);
	}
}

OutsideSerieGroup::~OutsideSerieGroup()
{
	for (const QMetaObject::Connection &connection : m_connections) {
		disconnect(connection);
	}
}

void OutsideSerieGroup::setName(const QString &name)
{
	if (m_name != name) {
		m_name = name;
		update();
	}
}

void OutsideSerieGroup::addSerie(Serie *serie)
{
	if (!m_series.contains(serie)) {
		m_series.append(serie);
		m_connections << connect(serie, &Serie::destroyed, [this, serie]() {
			m_series.removeOne(serie);
		});
		serie->addToSerieGroup(this);
		update();
	}
}

void OutsideSerieGroup::show(bool show)
{
	if (m_show != show) {
		m_show = show;
		update();
	}
}

void OutsideSerieGroup::hide()
{
	show(false);
}

void OutsideSerieGroup::setSerieSpacing(int spacing)
{
	if (m_spacing != spacing) {
		m_spacing = spacing;
		update();
	}
}

void OutsideSerieGroup::setMinimumHeight(int height)
{
	if (m_minimumHeight != height) {
		m_minimumHeight = height;
		update();
	}
}

void OutsideSerieGroup::setBackgroundColor(const QColor &color)
{
	if (m_backgroundColor != color) {
		m_backgroundColor = color;
		update();
	}
}

void OutsideSerieGroup::update()
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
