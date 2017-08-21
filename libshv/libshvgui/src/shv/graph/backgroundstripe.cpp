#include "backgroundstripe.h"

#include "view.h"
#include "serie.h"

namespace shv {
namespace gui {
namespace graphview {

BackgroundStripe::BackgroundStripe(QObject *parent) : BackgroundStripe(0, 0, OutlineType::No, parent)
{
}

BackgroundStripe::BackgroundStripe(ValueChange::ValueY min, ValueChange::ValueY max, OutlineType outline, QObject *parent)
	: QObject(parent)
	, m_min(min)
	, m_max(max)
	, m_outline(outline)
{
	Serie *serie = qobject_cast<Serie*>(parent);
	if (serie) {
		serie->addBackgroundStripe(this);
	}
}

void BackgroundStripe::setMin(const ValueChange::ValueY &min)
{
	setRange(min, m_max);
}

void BackgroundStripe::setMax(const ValueChange::ValueY &max)
{
	setRange(m_min, max);
}

void BackgroundStripe::setRange(const ValueChange::ValueY &min, const ValueChange::ValueY &max)
{
	m_min = min;
	m_max = max;
	update();
}

void BackgroundStripe::setOutlineType(BackgroundStripe::OutlineType outline)
{
	if (m_outline != outline) {
		m_outline = outline;
		update();
	}
}

void BackgroundStripe::update()
{
	View *graph = qobject_cast<View*>(parent());
	if (graph && graph->settings.showBackgroundStripes) {
		graph->update();
	}
}

}
}
}
