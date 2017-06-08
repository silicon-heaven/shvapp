#include "graphmodel.h"

namespace shv {
namespace gui {

GraphModel::GraphModel(QObject *parent) : QObject(parent)
  , m_dataAdded(false)
  , m_dataChangeEnabled(true)
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
	return compareValueX(value1.valueX, value2.valueX, type);
}

bool compareValueX(const ValueChange::ValueX &value1, const ValueChange::ValueX &value2, ValueType type)
{
	switch (type) {
	case ValueType::TimeStamp:
		return value1.timeStamp == value2.timeStamp;
	case ValueType::Double:
		return value1.doubleValue == value2.doubleValue;
	case ValueType::Int:
		return value1.intValue == value2.intValue;
	default:
		throw std::runtime_error("Invalid type on valueX");
	}
}

bool compareValueY(const ValueChange &value1, const ValueChange &value2, ValueType type)
{
	return compareValueY(value1.valueY, value2.valueY, type);
}

bool compareValueY(const ValueChange::ValueY &value1, const ValueChange::ValueY &value2, ValueType type)
{
	switch (type) {
	case ValueType::Double:
		return value1.doubleValue == value2.doubleValue;
	case ValueType::Int:
		return value1.intValue == value2.intValue;
	case ValueType::Bool:
		return value1.boolValue == value2.boolValue;
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
		added = addValueChangeInternal(serie_index, value) || added;
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
		added = addValueChangeInternal(i, values[i]) || added;
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
	if (serie.size() == 0 || (!compareValueX(serie.back(), value, serie.xType()) && !compareValueY(serie.back(), value, serie.yType()))) {
		serie.push_back(value);
		return true;
	}
	return false;
}

std::vector<ValueChange>::const_iterator SerieData::lessOrEqualIterator(quint64 msec_since_epoch) const
{
	auto it = std::lower_bound(cbegin(), cend(), msec_since_epoch,
							[](const ValueChange &val, double x) -> bool { return val.valueX.timeStamp < x; });

	if(it == cend()) {
		if(!empty())
			it--;
	}
	else {
		if (it-> valueX.timeStamp!= msec_since_epoch) {
			if(it == cbegin())
				it = cend();
			else
				it--;
		}
	}
	return it;
}

QPair<std::vector<ValueChange>::const_iterator, std::vector<ValueChange>::const_iterator> SerieData::intersection(const ValueChange::ValueX &start, const ValueChange::ValueX &end, bool &valid) const
{
	QPair<std::vector<ValueChange>::const_iterator, std::vector<ValueChange>::const_iterator> result;
	result.first = lessOrEqualIterator(start.timeStamp);
	result.second = lessOrEqualIterator(end.timeStamp);

	if ((result.first == cend()) && (!empty())){
		result.first = cbegin();
	}

	valid = (result.second != cend());
	return result;
}

}
}
