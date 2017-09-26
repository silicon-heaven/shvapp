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
	enum class Type { Vertical, Horizontal };

	BackgroundStripe(Type type, QObject *parent = 0);
	BackgroundStripe(Type type, OutlineType outline, QObject *parent = 0);
	BackgroundStripe(Type type, OutlineType outline, const QColor &stripeColor, QObject *parent = 0);
	BackgroundStripe(ValueChange::ValueY min, ValueChange::ValueY max, OutlineType outline, QObject *parent = 0);
	BackgroundStripe(ValueChange::ValueX min, ValueChange::ValueX max, OutlineType outline, QObject *parent = 0);
	BackgroundStripe(Type type, ValueChange min, ValueChange max, OutlineType outline, const QColor &stripe_color,
					 const QColor &outline_color, QObject *parent = 0);

	inline Type type() const { return m_type; }
	inline const ValueChange &min() const { return m_min; }
	inline const ValueChange &max() const { return m_max; }
	inline OutlineType outLineType() const { return m_outline; }
	inline const QColor &stripeColor() const { return m_stripeColor; }
	inline const QColor &outlineColor() const { return m_outlineColor; }

	void setMin(const ValueChange::ValueY &min);
	void setMax(const ValueChange::ValueY &max);
	void setRange(const ValueChange::ValueY &min, const ValueChange::ValueY &max);
	void setMin(const ValueChange::ValueX &min);
	void setMax(const ValueChange::ValueX &max);
	void setRange(const ValueChange::ValueX &min, const ValueChange::ValueX &max);
	void setMin(const ValueChange &min);
	void setMax(const ValueChange &max);
	void setRange(const ValueChange &min, const ValueChange &max);
	void setRange(const ValueXInterval &range);
	void setOutlineType(OutlineType outline);
	void setStripeColor(const QColor &color);
	void setOutlineColor(const QColor &color);

private:
	void update();

	Type m_type;
	ValueChange m_min = { 0LL, 0 };
	ValueChange m_max = { 0LL, 0 };
	OutlineType m_outline = OutlineType::No;
	QColor m_stripeColor;
	QColor m_outlineColor;
};

}
}
}
