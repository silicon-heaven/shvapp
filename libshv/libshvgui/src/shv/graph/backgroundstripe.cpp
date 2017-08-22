#include "backgroundstripe.h"

#include "graphview.h"
#include "serie.h"

namespace shv {
namespace gui {
namespace graphview {

BackgroundStripe::BackgroundStripe(QObject *parent) : BackgroundStripe(0, 0, parent)
{
}

BackgroundStripe::BackgroundStripe(ValueChange::ValueY min, ValueChange::ValueY max, QObject *parent)
	: QObject(parent)
	, m_min(min)
	, m_max(max)
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

	GraphView *graph = qobject_cast<GraphView*>(parent());
	if (graph && graph->settings.showBackgroundStripes) {
		graph->update();
	}
}

}
}
}
