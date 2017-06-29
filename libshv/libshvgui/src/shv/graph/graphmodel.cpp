#include "graphmodel.h"

#include "float.h"

#include <QDebug>

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
	return m_series[serie_index];
}

void GraphModel::checkIndex(int serie_index) const
{
	if (serie_index >= (int)m_series.size()) {
		throw std::runtime_error("bad serie index");
	}
}

ValueXInterval GraphModel::intRange() const
{
	ValueXInterval range(INT_MAX, INT_MIN);

	for (const SerieData *serie : m_series) {
		if (serie->xType() != ValueType::Int) {
			throw std::runtime_error("Cannot determine data range when X types are different");
		}

		ValueXInterval serie_range = serie->range();
		if (serie_range.min.intValue < range.min.intValue) {
			range.min.intValue = serie_range.min.intValue;
		}
		if (serie_range.max.intValue > range.max.intValue) {
			range.max.intValue = serie_range.max.intValue;
		}
	}
	if (range.min.intValue == INT_MAX) {
		range = ValueXInterval(0, 0);
	}

	return range;
}

ValueXInterval GraphModel::doubleRange() const
{
	ValueXInterval range(DBL_MAX, DBL_MIN);

	for (const SerieData *serie : m_series) {
		if (serie->xType() != ValueType::Double) {
			throw std::runtime_error("Cannot determine data range when X types are different");
		}

		ValueXInterval serie_range = serie->range();
		if (serie_range.min.doubleValue < range.min.doubleValue) {
			range.min.doubleValue = serie_range.min.doubleValue;
		}
		if (serie_range.max.doubleValue > range.max.doubleValue) {
			range.max.doubleValue = serie_range.max.doubleValue;
		}
	}
	if (range.min.doubleValue == DBL_MAX) {
		range = ValueXInterval(0.0, 0.0);
	}

	return range;
}

ValueXInterval GraphModel::timeStampRange() const
{
	ValueXInterval range((ValueChange::TimeStamp)INT64_MAX, (ValueChange::TimeStamp)INT64_MIN);

	for (const SerieData *serie : m_series) {
		if (serie->xType() != ValueType::TimeStamp) {
			throw std::runtime_error("Cannot determine data range when X types are different");
		}

		ValueXInterval serie_range = serie->range();

		if (serie_range.min.timeStamp < range.min.timeStamp) {
			range.min.timeStamp = serie_range.min.timeStamp;
		}
		if (serie_range.max.timeStamp > range.max.timeStamp) {
			range.max.timeStamp = serie_range.max.timeStamp;
		}
	}
	if (range.min.timeStamp == INT64_MAX) {
		range = ValueXInterval((ValueChange::TimeStamp)0, (ValueChange::TimeStamp)0);
	}

	return range;
}

SerieData *GraphModel::serieData(int serie_index)
{
	checkIndex(serie_index);
	return m_series[serie_index];
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

void GraphModel::addSerie(SerieData *serie)
{
	m_series.push_back(serie);
}

void GraphModel::clearSeries()
{
	for (SerieData *serie : m_series) {
		serie->clear();
		serie->shrink_to_fit();
	}
	Q_EMIT dataChanged();
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

ValueXInterval GraphModel::range() const
{
	if (!m_series.size()) {
		throw std::runtime_error("Cannot state range where no series are present");
	}
	ValueType type = m_series[0]->xType();
	switch (type) {
	case ValueType::Double:
		return doubleRange();
	case ValueType::Int:
		return intRange();
	case ValueType::TimeStamp:
		return timeStampRange();
	default:
		throw std::runtime_error("Invalid X axis type");
	}
}

bool GraphModel::addValueChangeInternal(int serie_index, const shv::gui::ValueChange &value)
{
	SerieData *serie = m_series[serie_index];
	return serie->addValueChange(value);
}

std::vector<ValueChange>::const_iterator SerieData::lessOrEqualIterator(ValueChange::ValueX value_x) const
{
	std::vector<ValueChange>::const_iterator it;
	if (m_xType == ValueType::TimeStamp) {
		it = std::lower_bound(cbegin(), cend(), value_x.timeStamp, [this](const ValueChange &val, const ValueChange::TimeStamp &x) -> bool {
			return val.valueX.timeStamp < x;
		});
	}
	else if (m_xType == ValueType::Int) {
		it = std::lower_bound(cbegin(), cend(), value_x.intValue, [this](const ValueChange &val, int x) -> bool {
			return val.valueX.intValue < x;
		});
	}
	else if (m_xType == ValueType::Double) {
		it = std::lower_bound(cbegin(), cend(), value_x.doubleValue, [this](const ValueChange &val, double x) -> bool {
			return val.valueX.doubleValue < x;
		});
	}

	if (it == cend()) {
		if (!empty()) {
			it--;
		}
	}
	else {
		if (!compareValueX(it->valueX, value_x, m_xType)) {
			if (it == cbegin()) {
				it = cend();
			}
			else {
				it--;
			}
		}
	}
	return it;
}

QPair<std::vector<ValueChange>::const_iterator, std::vector<ValueChange>::const_iterator> SerieData::intersection(const ValueChange::ValueX &start, const ValueChange::ValueX &end, bool &valid) const
{
	QPair<std::vector<ValueChange>::const_iterator, std::vector<ValueChange>::const_iterator> result;
	result.first = lessOrEqualIterator(start);
	result.second = lessOrEqualIterator(end);

	if ((result.first == cend()) && (!empty())){
		result.first = cbegin();
	}

	valid = (result.second != cend());
	return result;
}

ValueXInterval SerieData::range() const
{
	if (size()) {
		return ValueXInterval { at(0).valueX, back().valueX };
	}
	else {
		switch (xType()) {
		case ValueType::Double:
			return ValueXInterval { ValueChange::ValueX(0.0), ValueChange::ValueX(0.0) };
		case ValueType::Int:
			return ValueXInterval { ValueChange::ValueX(0), ValueChange::ValueX(0) };
		case ValueType::TimeStamp:
			return ValueXInterval { ValueChange::ValueX(0LL), ValueChange::ValueX(0LL) };
		default:
			throw std::runtime_error("Invalid type on X axis");
		}
	}
}

bool SerieData::addValueChange(const ValueChange &value)
{
	int sz = size();
	if (sz == 0) {
		push_back(value);
		return true;
	}
	else {
		const ValueChange &last = at(sz - 1);
		if (!compareValueX(last, value, xType()) && !compareValueY(last, value, yType())) {
			push_back(value);
			return true;
		}
	}
	return false;
}

}
}
