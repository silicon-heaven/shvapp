#ifndef SIMPLETEXTITEM_H
#define SIMPLETEXTITEM_H

#include "saxhandler.h"

#include <QGraphicsItem>

namespace svgscene {

class SimpleTextItem : public QGraphicsSimpleTextItem
{
	using Super = QGraphicsSimpleTextItem;
public:
	SimpleTextItem(const CssAttributes &css, QGraphicsItem *parent = nullptr);

	void setText(const QString text);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
private:
	int m_alignment = Qt::AlignLeft;
	QTransform m_origTransform;
	bool m_origTransformLoaded = false;
};

} // namespace svgscene

#endif // SIMPLETEXTITEM_H
