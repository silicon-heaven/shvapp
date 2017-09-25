#include "backgroundstripe.h"

#include <shv/core/shvexception.h>

#include "view.h"
#include "serie.h"

namespace shv {
namespace gui {
namespace graphview {

BackgroundStripe::BackgroundStripe(Type type, QObject *parent)
	: BackgroundStripe(type, { 0LL, 0 }, { 0LL, 0 }, OutlineType::No, QColor(), QColor(), parent)
{
}

BackgroundStripe::BackgroundStripe(BackgroundStripe::Type type, BackgroundStripe::OutlineType outline, QObject *parent)
	: BackgroundStripe(type, { 0LL, 0 }, { 0LL, 0 }, outline, QColor(), QColor(), parent)
{
}

BackgroundStripe::BackgroundStripe(BackgroundStripe::Type type, BackgroundStripe::OutlineType outline, const QColor &color, QObject *parent)
	: BackgroundStripe(type, { 0LL, 0 }, { 0LL, 0 }, outline, color, QColor(), parent)
{

}

BackgroundStripe::BackgroundStripe(ValueChange::ValueY min, ValueChange::ValueY max, OutlineType outline, QObject *parent)
	: BackgroundStripe(Type::Horizontal, { 0LL, min }, { 0LL, max }, outline, QColor(), QColor(), parent)
{
}

BackgroundStripe::BackgroundStripe(ValueChange::ValueX min, ValueChange::ValueX max, OutlineType outline, QObject *parent)
	: BackgroundStripe(Type::Vertical, { min, 0 }, { max, 0 }, outline, QColor(), QColor(), parent)
{
}

BackgroundStripe::BackgroundStripe(BackgroundStripe::Type type, ValueChange min, ValueChange max, BackgroundStripe::OutlineType outline, const QColor &stripe_color, const QColor &outline_color, QObject *parent)
	: QObject(parent)
	, m_type(type)
	, m_min(min)
	, m_max(max)
	, m_outline(outline)
	, m_stripeColor(stripe_color)
	, m_outlineColor(outline_color)
{
	Serie *serie = qobject_cast<Serie*>(parent);
	if (serie) {
		serie->addBackgroundStripe(this);
	}
	else {
		View *view = qobject_cast<View*>(parent);
		if (view) {
			view->addBackgroundStripe(this);
		}
	}
}

void BackgroundStripe::setMin(const ValueChange::ValueY &min)
{
	setRange(min, m_max.valueY);
}

void BackgroundStripe::setMax(const ValueChange::ValueY &max)
{
	setRange(m_min.valueY, max);
}

void BackgroundStripe::setRange(const ValueChange::ValueY &min, const ValueChange::ValueY &max)
{
	if (m_type != Type::Horizontal) {
		SHV_EXCEPTION("Bad background type");
	}
	setRange({ 0LL, min }, { 0LL, max });
}

void BackgroundStripe::setMin(const ValueChange::ValueX &min)
{
	setRange(min, m_max.valueX);
}

void BackgroundStripe::setMax(const ValueChange::ValueX &max)
{
	setRange(m_min.valueX, max);
}

void BackgroundStripe::setRange(const ValueChange::ValueX &min, const ValueChange::ValueX &max)
{
	if (m_type != Type::Vertical) {
		SHV_EXCEPTION("Bad background type");
	}
	setRange({ min, 0 }, { max, 0 });
}

void BackgroundStripe::setMin(const ValueChange &min)
{
	setRange(min, m_max);
}

void BackgroundStripe::setMax(const ValueChange &max)
{
	setRange(m_min, max);
}

void BackgroundStripe::setRange(const ValueChange &min, const ValueChange &max)
{
	m_min = min;
	m_max = max;
	update();
}

void BackgroundStripe::setRange(const View::XAxisInterval &range)
{
	setRange(range.start, range.end);
}

void BackgroundStripe::setOutlineType(BackgroundStripe::OutlineType outline)
{
	if (m_outline != outline) {
		m_outline = outline;
		update();
	}
}

void BackgroundStripe::setStripeColor(const QColor &color)
{
	if (m_stripeColor != color) {
		m_stripeColor = color;
		update();
	}
}

void BackgroundStripe::setOutlineColor(const QColor &color)
{
	if (m_outlineColor != color) {
		m_outlineColor = color;
		update();
	}
}

void BackgroundStripe::update()
{
	View *graph = qobject_cast<View*>(parent());
	if (graph) {
		graph->update();
	}
	else {
		Serie *serie = qobject_cast<Serie*>(parent());
		if (serie) {
			View *graph = serie->view();
			if (graph && graph->settings.showSerieBackgroundStripes) {
				graph->update();
			}
		}
	}
}

}
}
}
