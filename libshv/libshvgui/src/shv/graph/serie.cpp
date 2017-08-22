#include "serie.h"

#include "backgroundstripe.h"
#include "view.h"
#include "outsideseriegroup.h"

namespace shv {
namespace gui {
namespace graphview {

Serie::Serie(ValueType type, int serieIndex, const QString &name, const QColor &color, QObject *parent)
	: QObject(parent)
	, m_name(name)
	, m_type(type)
	, m_color(color)
	, m_serieIndex(serieIndex)
{
	View *graph = qobject_cast<View*>(parent);
	if (graph) {
		graph->addSerie(this);
	}
	else {
		Serie *serie = qobject_cast<Serie*>(parent);
		if (serie) {
			serie->addDependentSerie(this);
		}
	}
}

Serie::~Serie()
{
	for (const QMetaObject::Connection &connection : m_connections) {
		disconnect(connection);
	}
}

void Serie::setName(const QString &name)
{
	if (m_name != name) {
		m_name = name;
		update();
	}
}

Serie::YAxis Serie::relatedAxis() const
{
	const Serie *master_serie = masterSerie();
	if (master_serie) {
		return master_serie->m_relatedAxis;
	}
	return YAxis::Y1;
}

void Serie::setRelatedAxis(Serie::YAxis axis)
{
	if (masterSerie() != this) {
		throw std::runtime_error("Cannot set color on dependent serie");
	}
	if (m_relatedAxis != axis) {
		m_relatedAxis = axis;
		update();
	}
}

QColor Serie::color() const
{
	const Serie *master_serie = masterSerie();
	if (master_serie) {
		return master_serie->m_color;
	}
	return QColor();
}

void Serie::setColor(const QColor &color)
{
	if (masterSerie() != this) {
		throw std::runtime_error("Cannot set color on dependent serie");
	}
	if (m_color != color) {
		m_color = color;
		update();
	}
}

void Serie::addBackgroundStripe(BackgroundStripe *stripe)
{
	if (!m_backgroundStripes.contains(stripe)) {
		m_backgroundStripes.append(stripe);
		stripe->setParent(this);
		m_connections << connect(stripe, &BackgroundStripe::destroyed, [this, stripe]() {
			m_backgroundStripes.removeOne(stripe);
			update();
		});
		update();
	}
}

void Serie::setLineWidth(int width)
{
	if (m_lineWidth != width) {
		m_lineWidth = width;
		update();
	}
}

const QVector<Serie *> &Serie::dependentSeries() const
{
	return m_dependentSeries;
}

void Serie::addDependentSerie(Serie *serie)
{
	if (!m_dependentSeries.contains(serie)) {
		m_dependentSeries.append(serie);
		m_connections << connect(serie, &Serie::destroyed, [this, serie] {
			m_dependentSeries.removeOne(serie);
			update();
		});
		update();
	}
}

void Serie::addToSerieGroup(OutsideSerieGroup *group)
{
	if (!m_serieGroup) {
		m_serieGroup = group;
		connect(group, &OutsideSerieGroup::destroyed, [this, group] {
			m_serieGroup = nullptr;
		});
		group->addSerie(this);
	}
}

void Serie::setLineType(Serie::LineType line_type)
{
	if (m_lineType != line_type) {
		m_lineType = line_type;
		update();
	}
}

void Serie::setLegendValueFormatter(std::function<QString (const ValueChange &)> formatter)
{
	m_legendValueFormatter = formatter;
	update();
}

void Serie::setValueFormatter(std::function<ValueChange::ValueY (const ValueChange &)> formatter)
{
	m_valueFormatter = formatter;
	update();
}

void Serie::setBoolValue(double value)
{
	if (m_boolValue != value) {
		m_boolValue = value;
		update();
	}
}

void Serie::show()
{
	if (!m_show) {
		m_show = true;
		update();
	}
}

void Serie::hide()
{
	if (m_show) {
		m_show = false;
		update();
	}
}

void Serie::setShowCurrent(bool show)
{
	if (m_showCurrent != show) {
		m_showCurrent = show;
		update();
	}
}

const SerieData &Serie::serieModelData(const View *view) const
{
	return serieModelData(view->model());
}

const SerieData &Serie::serieModelData(const GraphModel *model) const
{
	return model->serieData(m_serieIndex);
}

void Serie::update()
{
	if (m_show) {
		View *graph = view();
		if (graph) {
			graph->computeGeometry();
			graph->update();
		}
	}
}

const Serie *Serie::masterSerie() const
{
	const Serie *master = this;
	Serie *parent_serie = 0;
	while ((parent_serie = qobject_cast<Serie*>(master->parent()))) {
		master = parent_serie;
	}
	return master;
}

View *Serie::view() const
{
	return qobject_cast<View*>(masterSerie()->parent());
}

}
}
}
