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
	enum class Type { Vertical, Point };

	PointOfInterest(QObject *parent = 0);
	PointOfInterest(ValueChange::ValueX position, const QString &comment, const QColor &color, QObject *parent = 0); //vertical
	PointOfInterest(ValueChange position, const QString &comment, const QColor &color, Type poi_type, QObject *parent = 0);
	PointOfInterest(ValueChange position, const QString &comment, const QColor &color, Type type, Serie *serie, QObject *parent = 0);

	inline ValueChange position() const { return m_position; }
	inline const QString &comment() const  { return m_comment; }
	inline const QColor &color() const { return m_color; }
	inline Type type() const { return m_type; }
	inline Serie *serie() const { return m_serie; }

	void setPosition(ValueChange position);
	void setPosition(ValueChange::ValueX position);

private:
	void update();

	ValueChange m_position;
	QString m_comment;
	QColor m_color;
	Type m_type;
	Serie *m_serie;
};

}
}
}
