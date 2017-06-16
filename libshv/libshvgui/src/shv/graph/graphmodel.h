#pragma once

#include "../../shvguiglobal.h"

#include <QObject>
#include <QPair>
#include <memory>
#include <cmath>

namespace shv {
namespace gui {

enum class ValueType { TimeStamp, Int, Double, Bool };

struct SHVGUI_DECL_EXPORT ValueChange
{
	union ValueX {
		ValueX(qint64 value) : timeStamp(value) {}
		ValueX(int value) : intValue(value) {}
		ValueX(double value) : doubleValue(value) {}
		ValueX() : intValue(0) {}

		double toDouble(ValueType stored_type) const{
			switch (stored_type) {
				case ValueType::Int: return intValue;
				case ValueType::Double: return doubleValue;
				case ValueType::TimeStamp: return timeStamp;
				default: Q_ASSERT_X(false,"valueX", "Unsupported conversion"); return 0;
			}
		}

		int toInt(ValueType stored_type) const{
			switch (stored_type) {
				case ValueType::Int: return intValue;
				case ValueType::Double: return std::nearbyint(doubleValue);
				case ValueType::TimeStamp: return timeStamp;
				default: Q_ASSERT_X(false,"valueX", "Unsupported conversion"); return 0;
			}
		}

		qint64 timeStamp;
		int intValue;
		double doubleValue;
	} valueX;

	union ValueY {
		ValueY(bool value) : boolValue(value) {}
		ValueY(int value) : intValue(value) {}
		ValueY(double value) : doubleValue(value) {}
		ValueY() : intValue(0) {}

		double toDouble(ValueType stored_type) const{
			switch (stored_type) {
				case ValueType::Int: return intValue;
				case ValueType::Double: return doubleValue;
				case ValueType::Bool: return boolValue;
				default: Q_ASSERT_X(false,"valueY", "Unsupported conversion"); return 0;
			}
		}

		int toInt(ValueType stored_type) const{
			switch (stored_type) {
				case ValueType::Int: return intValue;
				case ValueType::Double: return std::nearbyint(doubleValue);
				case ValueType::Bool: return boolValue;
				default: Q_ASSERT_X(false,"valueY", "Unsupported conversion"); return 0;
			}
		}

		bool toBool(ValueType stored_type) const{
			switch (stored_type) {
				case ValueType::Int: return (intValue > 0);
				case ValueType::Double: return (doubleValue > 0.0);
				case ValueType::Bool: return boolValue;
				default: Q_ASSERT_X(false,"valueY", "Unsupported conversion"); return false;
			}
		}

		bool boolValue;
		int intValue;
		double doubleValue;
	} valueY;

	ValueChange(ValueX value_x, ValueY value_y) : valueX(value_x), valueY(value_y) {}
	ValueChange(qint64 value_x, ValueY value_y) : ValueChange(ValueX(value_x), value_y) {}
	ValueChange(qint64 value_x, bool value_y) : ValueChange(value_x, ValueY(value_y)) {}
	ValueChange(qint64 value_x, int value_y) : ValueChange(value_x, ValueY(value_y)) {}
	ValueChange(qint64 value_x, double value_y) : ValueChange(value_x, ValueY(value_y)) {}
	ValueChange() {}
};

SHVGUI_DECL_EXPORT bool compareValueX(const ValueChange &value1, const ValueChange &value2, ValueType type);
SHVGUI_DECL_EXPORT bool compareValueX(const ValueChange::ValueX &value1, const ValueChange::ValueX &value2, ValueType type);

SHVGUI_DECL_EXPORT bool compareValueY(const ValueChange &value1, const ValueChange &value2, ValueType type);
SHVGUI_DECL_EXPORT bool compareValueY(const ValueChange::ValueY &value1, const ValueChange::ValueY &value2, ValueType type);

class SHVGUI_DECL_EXPORT SerieData : public std::vector<ValueChange>
{
public:
	SerieData() : m_xType(ValueType::Int), m_yType(ValueType::Int)	{}
	SerieData(ValueType x_type, ValueType y_type) : m_xType(x_type), m_yType(y_type) {}
	std::vector<ValueChange>::const_iterator lessOrEqualIterator(quint64 msec_since_epoch) const;
	QPair<std::vector<ValueChange>::const_iterator, std::vector<ValueChange>::const_iterator> intersection(const ValueChange::ValueX &start, const ValueChange::ValueX &end, bool &valid) const;

	ValueType xType() const	{ return m_xType; }
	ValueType yType() const	{ return m_yType; }

private:
	ValueType m_xType;
	ValueType m_yType;
};

struct SHVGUI_DECL_EXPORT GraphModel : public QObject
{
	Q_OBJECT

public:
	GraphModel(QObject *parent);

	Q_SIGNAL void dataChanged();

	virtual SerieData *serieData(int serie_index);
	virtual const SerieData *serieData(int serie_index) const;

	void addValueChange(int serie_index, const shv::gui::ValueChange &value);
	void addValueChanges(int serie_index, const std::vector<shv::gui::ValueChange> &values); // adds array of valyes to one serie
	void addValueChanges(const std::vector<shv::gui::ValueChange> &values); //adds array of values where every value belongs to one serie

	void addSerie(ValueType xType, ValueType yType);
	void clearSeries();

	void addDataBegin();
	void addDataEnd();

protected:
	void checkIndex(int serie_index) const;
	virtual bool addValueChangeInternal(int serie_index, const shv::gui::ValueChange &value);

	std::vector<SerieData> m_series;
	bool m_dataAdded;
	bool m_dataChangeEnabled;
};

}
}
