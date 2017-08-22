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
	BackgroundStripe(QObject *parent = 0);
	BackgroundStripe(ValueChange::ValueY min, ValueChange::ValueY max, QObject *parent = 0);

	inline const ValueChange::ValueY &min() const { return m_min; }
	inline const ValueChange::ValueY &max() const { return m_max; }

	void setMin(const ValueChange::ValueY &min);
	void setMax(const ValueChange::ValueY &max);
	void setRange(const ValueChange::ValueY &min, const ValueChange::ValueY &max);

private:
	ValueChange::ValueY m_min = 0;
	ValueChange::ValueY m_max = 0;
};

}
}
}
