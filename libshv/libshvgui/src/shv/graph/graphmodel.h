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
		double realValue;
	} valueX;
	union {
		bool boolValue;
		int intValue;
		double realValue;
	} valueY;
};

class SerieData : public std::vector<ValueChange>
{
public:
	enum class Type { TimeStamp, Int, Real, Bool };

	Type xType;
	Type yType;
};

struct SHVGUI_DECL_EXPORT GraphModel : public QObject
{
	Q_OBJECT

public:
	GraphModel(QObject *parent);

	Q_SIGNAL void dataChanged();

	virtual SerieData *serieData(int serie_index) = 0;
	virtual const SerieData *serieData(int serie_index) const = 0;
};

}
}
