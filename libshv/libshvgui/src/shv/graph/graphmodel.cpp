#include "graphmodel.h"

namespace shv {
namespace gui {

GraphModel::GraphModel(QObject *parent) : QObject(parent)
{

}

const SerieData *GraphModel::serieData(int serie_index) const
{
	checkIndex(serie_index);
	return &series[serie_index];
}

void GraphModel::checkIndex(int serie_index) const
{
	if (serie_index >= (int)series.size()) {
		throw std::runtime_error("bad serie index");
	}
}

SerieData *GraphModel::serieData(int serie_index)
{
	checkIndex(serie_index);
	return &series[serie_index];
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
	if (addValueChangeInternal(serie_index, value)) {
		Q_EMIT dataChanged();
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
		Q_EMIT dataChanged();
	}
}

void GraphModel::addValueChanges(const std::vector<ValueChange> &values)
{
	if (values.size() != series.size()) {
		throw std::runtime_error("addValueChanges: number of values in array doesn't match number of series");
	}
	bool added = false;
	for (uint i = 0; i < series.size(); ++i) {
		added = added || addValueChangeInternal(i, values[i]);
	}
	if (added) {
		Q_EMIT dataChanged();
	}
}

void GraphModel::addSerie(ValueType xType, ValueType yType)
{
	series.emplace_back(xType, yType);
}

bool GraphModel::addValueChangeInternal(int serie_index, const shv::gui::ValueChange &value)
{
	SerieData &serie = series[serie_index];
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
