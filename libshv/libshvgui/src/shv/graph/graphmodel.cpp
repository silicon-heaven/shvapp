#include "graphmodel.h"

#include "float.h"

#include <shv/core/shvexception.h>

#include <QDebug>

namespace shv {
namespace gui {

SerieData::const_iterator SerieData::lessOrEqualIterator(ValueChange::ValueX value_x) const
{
	const_iterator it = lower_bound(value_x);

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

QPair<SerieData::const_iterator, SerieData::const_iterator> SerieData::intersection(const ValueChange::ValueX &start, const ValueChange::ValueX &end, bool &valid) const
{
	QPair<const_iterator, const_iterator> result;
	result.first = lessOrEqualIterator(start);
	result.second = lessOrEqualIterator(end);

	if (result.first == cend() && !empty()){
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
			SHV_EXCEPTION("Invalid type on X axis");
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

void GraphModelData::checkIndex(int serie_index) const
{
	if (serie_index >= (int)m_valueChanges.size()) {
		SHV_EXCEPTION("bad serie index");
	}
}

ValueXInterval GraphModelData::intRange() const
{
	ValueXInterval range(INT_MAX, INT_MIN);

	for (const SerieData &serie : m_valueChanges) {
		if (serie.xType() != ValueType::Int) {
			SHV_EXCEPTION("Cannot determine data range when X types are different");
		}

		ValueXInterval serie_range = serie.range();
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

ValueXInterval GraphModelData::doubleRange() const
{
	ValueXInterval range(DBL_MAX, DBL_MIN);

	for (const SerieData &serie : m_valueChanges) {
		if (serie.xType() != ValueType::Double) {
			SHV_EXCEPTION("Cannot determine data range when X types are different");
		}

		ValueXInterval serie_range = serie.range();
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

ValueXInterval GraphModelData::timeStampRange() const
{
	ValueXInterval range((ValueChange::TimeStamp)INT64_MAX, (ValueChange::TimeStamp)INT64_MIN);

	for (const SerieData &serie : m_valueChanges) {
		if (serie.xType() != ValueType::TimeStamp) {
			SHV_EXCEPTION("Cannot determine data range when X types are different");
		}
		if (serie.size()) {
			ValueXInterval serie_range = serie.range();

			if (serie_range.min.timeStamp < range.min.timeStamp) {
				range.min.timeStamp = serie_range.min.timeStamp;
			}
			if (serie_range.max.timeStamp > range.max.timeStamp) {
				range.max.timeStamp = serie_range.max.timeStamp;
			}
		}
	}
	if (range.min.timeStamp == INT64_MAX) {
		range = ValueXInterval((ValueChange::TimeStamp)0, (ValueChange::TimeStamp)0);
	}

	return range;
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
		SHV_EXCEPTION("Invalid type on valueX");
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
		SHV_EXCEPTION("Invalid type on valueY");
	}
}

GraphModelData::GraphModelData(QObject *parent)
	: QObject(parent)
	, m_dataChangeEnabled(true)
{
}

SerieData &GraphModelData::serieData(int serie_index)
{
	checkIndex(serie_index);
	return m_valueChanges[serie_index];
}

const SerieData &GraphModelData::serieData(int serie_index) const
{
	checkIndex(serie_index);
	return m_valueChanges[serie_index];
}

void GraphModelData::addValueChange(int serie_index, const shv::gui::ValueChange &value)
{
	bool added = addValueChangeInternal(serie_index, value);
	if (added) {
		if (m_dataChangeEnabled) {
			Q_EMIT dataChanged(QVector<int>{ serie_index });
		}
		else if (!m_changedSeries.contains(serie_index)){
			m_changedSeries << serie_index;
		}
	}
}

void GraphModelData::addValueChanges(int serie_index, const std::vector<shv::gui::ValueChange> &values)
{
	bool added = false;
	for (const shv::gui::ValueChange &value : values) {
		added = addValueChangeInternal(serie_index, value) || added;
	}
	if (added) {
		if (m_dataChangeEnabled) {
			Q_EMIT dataChanged(QVector<int>{ serie_index });
		}
		else if (!m_changedSeries.contains(serie_index)){
			m_changedSeries << serie_index;
		}
	}
}

void GraphModelData::addValueChanges(const std::vector<ValueChange> &values)
{
	if (values.size() >= m_valueChanges.size()) {
		SHV_EXCEPTION("addValueChanges: number of values in array exceeds the number of series");
	}
	QVector<int> added;
	for (uint i = 0; i < m_valueChanges.size(); ++i) {
		if (addValueChangeInternal(i, values[i])) {
			added << i;
		}
	}
	if (added.count()) {
		if (m_dataChangeEnabled) {
			Q_EMIT dataChanged(added);
		}
		else {
			for (int serie_index : added) {
				if (!m_changedSeries.contains(serie_index)) {
					m_changedSeries << serie_index;
				}
			}
		}
	}
}

void GraphModelData::addSerie(SerieData values)
{
	m_valueChanges.push_back(values);
}

void GraphModelData::clearSerie(int serie_index)
{
	checkIndex(serie_index);
	SerieData &serie = m_valueChanges[serie_index];
	if (serie.size()) {
		serie.clear();
		serie.shrink_to_fit();
		if (m_dataChangeEnabled) {
			Q_EMIT dataChanged(QVector<int>{ serie_index });
		}
		else if (!m_changedSeries.contains(serie_index)){
			m_changedSeries << serie_index;
		}
	}
}

void GraphModelData::clearSeries()
{
	QVector<int> changed;
	for (uint serie_index = 0; serie_index < m_valueChanges.size(); ++serie_index) {
		SerieData &serie = m_valueChanges[serie_index];
		if (serie.size()) {
			changed << serie_index;
			serie.clear();
		}
		serie.shrink_to_fit();
	}
	if (changed.size()) {
		if (m_dataChangeEnabled) {
			Q_EMIT dataChanged(changed);
		}
		else {
			for (int serie_index : changed) {
				if (!m_changedSeries.contains(serie_index)) {
					m_changedSeries << serie_index;
				}
			}
		}
	}
}

void GraphModelData::dataChangeBegin()
{
	m_dataChangeEnabled = false;
}

void GraphModelData::dataChangeEnd()
{
	if (!m_dataChangeEnabled && m_changedSeries.size()) {
		Q_EMIT dataChanged(m_changedSeries);
	}
	m_changedSeries.clear();
	m_dataChangeEnabled = true;
}

SerieData::iterator GraphModelData::removeValueChanges(int serie_index, SerieData::const_iterator from, SerieData::const_iterator to)
{
	SerieData &serie = serieData(serie_index);
	auto old_size = serie.size();
	auto it = serie.erase(from, to);
	if (serie.size() != old_size) {
		if (m_dataChangeEnabled) {
			Q_EMIT dataChanged(QVector<int>{ serie_index });
		}
		else if (!m_changedSeries.contains(serie_index)) {
			m_changedSeries << serie_index;
		}
	}
	return it;
}

ValueXInterval GraphModelData::range() const
{
	if (!m_valueChanges.size()) {
		SHV_EXCEPTION("Cannot state range where no series are present");
	}
	ValueType type = m_valueChanges[0].xType();
	switch (type) {
	case ValueType::Double:
		return doubleRange();
	case ValueType::Int:
		return intRange();
	case ValueType::TimeStamp:
		return timeStampRange();
	default:
		SHV_EXCEPTION("Invalid X axis type");
	}
}

bool GraphModelData::addValueChangeInternal(int serie_index, const shv::gui::ValueChange &value)
{
	SerieData &serie = serieData(serie_index);
	return serie.addValueChange(value);
}

GraphModel::GraphModel(QObject *parent)
	: QObject(parent)
{
}

void GraphModel::setData(GraphModelData *model_data)
{
	if(m_data)
		disconnect(m_data, 0, this, 0);
	m_data = model_data;
	connect(m_data, &GraphModelData::dataChanged, this, &GraphModel::dataChanged);
	connect(m_data, &GraphModelData::destroyed, this, &GraphModel::deleteLater);
}

GraphModelData *GraphModel::data() const
{
	if(!m_data)
		SHV_EXCEPTION("No data set!");
	return m_data;
}

SerieData &GraphModel::serieData(int serie_index)
{
	return data()->serieData(serie_index);
}

const SerieData &GraphModel::serieData(int serie_index) const
{
	return data()->serieData(serie_index);
}

SerieData::const_iterator SerieData::upper_bound(ValueChange::ValueX value_x) const
{
	const_iterator it = end();
	if (m_xType == ValueType::TimeStamp) {
		it = std::upper_bound(cbegin(), cend(), value_x.timeStamp, [](const ValueChange::TimeStamp &x, const ValueChange &val) {
			return val.valueX.timeStamp > x;
		});
	}
	else if (m_xType == ValueType::Int) {
		it = std::upper_bound(cbegin(), cend(), value_x.intValue, [](int x, const ValueChange &val) {
			return val.valueX.intValue > x;
		});
	}
	else if (m_xType == ValueType::Double) {
		it = std::upper_bound(cbegin(), cend(), value_x.doubleValue, [](double x, const ValueChange &val) {
			return val.valueX.doubleValue > x;
		});
	}
	return it;
}

SerieData::const_iterator SerieData::lower_bound(ValueChange::ValueX value_x) const
{
	const_iterator it = end();
	if (m_xType == ValueType::TimeStamp) {
		it = std::lower_bound(cbegin(), cend(), value_x.timeStamp, [](const ValueChange &val, const ValueChange::TimeStamp &x) {
			return val.valueX.timeStamp < x;
		});
	}
	else if (m_xType == ValueType::Int) {
		it = std::lower_bound(cbegin(), cend(), value_x.intValue, [](const ValueChange &val, int x) {
			return val.valueX.intValue < x;
		});
	}
	else if (m_xType == ValueType::Double) {
		it = std::lower_bound(cbegin(), cend(), value_x.doubleValue, [](const ValueChange &val, double x) {
			return val.valueX.doubleValue < x;
		});
	}
	return it;
}

}
}
