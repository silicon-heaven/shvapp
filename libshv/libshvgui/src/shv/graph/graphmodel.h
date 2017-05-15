#pragma once

#include "../../shvguiglobal.h"

#include <QVector>
#include <memory>

namespace shv {
namespace gui {

struct SHVGUI_DECL_EXPORT SerieType
{
	enum Enum {
		Osc = 0,
		Amp,
		Cos,
		Sin,
		P3,
		P4,
		VehicleType,
		VehicleEvent,
		BlockedOsc,
		BlockedAmp,
		BlockedCos,
		BlockedSin,
		BlockedP3,
		BlockedP4,
		TypeCount
	};
};

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

using SerieData = std::vector<ValueChange>;
using SerieDataSharedPtr = std::shared_ptr<const SerieData>;

struct SHVGUI_DECL_EXPORT GraphModel
{
	SerieData series[(int)SerieType::TypeCount];// = {SerieData(), SerieData(), SerieData()};
};

}
}
