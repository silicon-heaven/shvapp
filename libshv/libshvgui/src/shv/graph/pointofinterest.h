#pragma once

#include "../../shvguiglobal.h"
#include "graphmodel.h"

#include <QColor>

namespace shv {
namespace gui {
namespace graphview {

class SHVGUI_DECL_EXPORT PointOfInterest : public QObject
{
	Q_OBJECT

public:
	PointOfInterest(QObject *parent = 0);
	PointOfInterest(ValueChange::ValueX position, const QString &comment, const QColor &color, QObject *parent = 0);

	ValueChange::ValueX position() const { return m_position; }
	const QString &comment() const  { return m_comment; }
	const QColor &color() const { return m_color; }

private:
	ValueChange::ValueX m_position;
	QString m_comment;
	QColor m_color;
};

}
}
}
