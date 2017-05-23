#include "graphmodel.h"

namespace shv {
namespace gui {

GraphModel::GraphModel(QObject *parent) : QObject(parent), m_dataAdded(false), m_dataChangeEnabled(true)
{

}

const SerieData *GraphModel::serieData(int serie_index) const
{
	checkIndex(serie_index);
	return &m_series[serie_index];
}

void GraphModel::checkIndex(int serie_index) const
{
	if (serie_index >= (int)m_series.size()) {
		throw std::runtime_error("bad serie index");
	}
}

SerieData *GraphModel::serieData(int serie_index)
{
	checkIndex(serie_index);
	return &m_series[serie_index];
}

bool compareValueX(const ValueChange &value1, const ValueChange &value2, ValueType type)
{
	switch (type) {
	case ValueType::TimeStamp:
		return value1.valueX.timeStamp == value2.valueX.timeStamp;
	case ValueType::Double:
		return value1.valueX.doubleValue == value2.valueX.doubleValue;
	case ValueType::Int:
		return value1.valueX.intValue == value2.valueX.intValue;
	default:
		throw std::runtime_error("Invalid type on valueX");
	}
}

bool compareValueY(const ValueChange &value1, const ValueChange &value2, ValueType type)
{
	switch (type) {
	case ValueType::Double:
		return value1.valueY.doubleValue == value2.valueY.doubleValue;
	case ValueType::Int:
		return value1.valueY.intValue == value2.valueY.intValue;
	case ValueType::Bool:
		return value1.valueY.boolValue == value2.valueY.boolValue;
	default:
		throw std::runtime_error("Invalid type on valueY");
	}
}

void GraphModel::addValueChange(int serie_index, const shv::gui::ValueChange &value)
{
	checkIndex(serie_index);
	bool added = addValueChangeInternal(serie_index, value);
	if (added) {
		if (m_dataChangeEnabled) {
			Q_EMIT dataChanged();
		}
		else {
			m_dataAdded = true;
		}
	}
}

void GraphModel::addValueChanges(int serie_index, const std::vector<shv::gui::ValueChange> &values)
{
	checkIndex(serie_index);
	bool added = false;
	for (const shv::gui::ValueChange &value : values) {
		added = added || addValueChangeInternal(serie_index, value);
	}
	if (added) {
		if (m_dataChangeEnabled) {
			Q_EMIT dataChanged();
		}
		else {
			m_dataAdded = true;
		}
	}
}

void GraphModel::addValueChanges(const std::vector<ValueChange> &values)
{
	if (values.size() != m_series.size()) {
		throw std::runtime_error("addValueChanges: number of values in array doesn't match number of series");
	}
	bool added = false;
	for (uint i = 0; i < m_series.size(); ++i) {
		added = added || addValueChangeInternal(i, values[i]);
	}
	if (added) {
		if (m_dataChangeEnabled) {
			Q_EMIT dataChanged();
		}
		else {
			m_dataAdded = true;
		}
	}
}

void GraphModel::addSerie(ValueType xType, ValueType yType)
{
	m_series.emplace_back(xType, yType);
}

void GraphModel::clearSeries()
{
	for (SerieData &serie : m_series) {
		serie.clear();
		serie.shrink_to_fit();
	}
}

void GraphModel::addDataBegin()
{
	m_dataChangeEnabled = false;
}

void GraphModel::addDataEnd()
{
	if (!m_dataChangeEnabled && m_dataAdded) {
		Q_EMIT dataChanged();
	}
	m_dataAdded = false;
	m_dataChangeEnabled = true;
}

bool GraphModel::addValueChangeInternal(int serie_index, const shv::gui::ValueChange &value)
{
	SerieData &serie = m_series[serie_index];
	if (serie.size() == 0 || !compareValueX(serie.back(), value, serie.xType()) || !compareValueY(serie.back(), value, serie.yType())) {
		serie.push_back(value);
		return true;
	}
	return false;
}

SerieData::SerieData(ValueType x_type, ValueType y_type) : m_xType(x_type), m_yType(y_type)
{
}

ValueType SerieData::xType() const
{
	return m_xType;
}

ValueType SerieData::yType() const
{
	return m_yType;
}

}
}
