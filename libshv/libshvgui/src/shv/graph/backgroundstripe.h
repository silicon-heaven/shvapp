#pragma once

#include "../../shvguiglobal.h"

#include "graphmodel.h"

#include <QColor>

namespace shv {
namespace gui {
namespace graphview {

class SHVGUI_DECL_EXPORT BackgroundStripe : public QObject
{
	Q_OBJECT

public:
	enum class OutlineType { No, Min, Max, Both };

	BackgroundStripe(QObject *parent = 0);
	BackgroundStripe(ValueChange::ValueY min, ValueChange::ValueY max, OutlineType outline, QObject *parent = 0);

	inline const ValueChange::ValueY &min() const { return m_min; }
	inline const ValueChange::ValueY &max() const { return m_max; }
	inline OutlineType outLineType() const { return m_outline; }

	void setMin(const ValueChange::ValueY &min);
	void setMax(const ValueChange::ValueY &max);
	void setRange(const ValueChange::ValueY &min, const ValueChange::ValueY &max);
	void setOutlineType(OutlineType outline);

private:
	void update();

	ValueChange::ValueY m_min = 0;
	ValueChange::ValueY m_max = 0;
	OutlineType m_outline = OutlineType::No;
};

}
}
}
