#pragma once

#include "../../shvguiglobal.h"

#include <QObject>
#include <memory>

namespace shv {
namespace gui {

struct SHVGUI_DECL_EXPORT ValueChange
{
	union {
		quint64 timeStamp;
		int intValue;
		double doubleValue;
	} valueX;
	union {
		bool boolValue;
		int intValue;
		double doubleValue;
	} valueY;
};

enum class ValueType { TimeStamp, Int, Double, Bool };

SHVGUI_DECL_EXPORT bool compareValueX(const ValueChange &value1, const ValueChange &value2, ValueType type);
SHVGUI_DECL_EXPORT bool compareValueY(const ValueChange &value1, const ValueChange &value2, ValueType type);

class SHVGUI_DECL_EXPORT SerieData : public std::vector<ValueChange>
{
public:
	SerieData(ValueType x_type, ValueType y_type);

	ValueType xType() const;
	ValueType yType() const;

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

protected:
	void checkIndex(int serie_index) const;
	virtual bool addValueChangeInternal(int serie_index, const shv::gui::ValueChange &value);

	std::vector<SerieData> series;
};

}
}
