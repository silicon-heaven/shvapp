#pragma once

#include "../../shvguiglobal.h"

#include <QColor>
#include <QObject>
#include <QVector>

namespace shv {
namespace gui {
namespace graphview {

class Serie;

class SHVGUI_DECL_EXPORT OutsideSerieGroup : public QObject
{
	Q_OBJECT

public:
	OutsideSerieGroup(QObject *parent = 0);
	OutsideSerieGroup(const QString &name, QObject *parent = 0);
	~OutsideSerieGroup();

	inline const QString &name() const { return m_name; }
	void setName(const QString &name);

	inline bool isHidden() const { return !m_show; }
	void show(bool show = true);
	void hide();

	inline int serieSpacing() const { return m_spacing; }
	void setSerieSpacing(int spacing);

	inline int minimumHeight() const { return m_minimumHeight; }
	void setMinimumHeight(int height);

	inline const QColor &backgroundColor() const { return m_backgroundColor; }
	void setBackgroundColor(const QColor &color);

	inline const QVector<Serie*> &series() const { return m_series; }
	void addSerie(Serie *serie);

private:
	void update();

	QString m_name;
	QVector<Serie*> m_series;
	int m_minimumHeight = 20;
	bool m_show = false;
	QColor m_backgroundColor;
	int m_spacing = 4;
	QVector<QMetaObject::Connection> m_connections;
};

}
}
}
