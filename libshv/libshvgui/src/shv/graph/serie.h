#pragma once

#include "../../shvguiglobal.h"
#include "graphmodel.h"

#include <QColor>
#include <QVector>

namespace shv {
namespace gui {
namespace graphview {

class View;
class BackgroundStripe;
class OutsideSerieGroup;

class SHVGUI_DECL_EXPORT Serie : public QObject
{
	Q_OBJECT

public:
	enum class LineType { OneDimensional, TwoDimensional };
	enum class YAxis { Y1, Y2 };

	Serie(ValueType type, int serieIndex, const QString &name, const QColor &color, QObject *parent = 0);
	~Serie();

	inline const QString &name() const { return m_name; }
	void setName(const QString &name);

	inline const ValueType &type() const  { return m_type; }

	YAxis relatedAxis() const;
	void setRelatedAxis(YAxis axis);

	QColor color() const;
	void setColor(const QColor &color);

	const QVector<BackgroundStripe*> &backgroundStripes() const  { return m_backgroundStripes; }
	void addBackgroundStripe(BackgroundStripe *stripe);

	inline int lineWidth() const  { return m_lineWidth; }
	void setLineWidth(int width);

	const QVector<Serie*> &dependentSeries() const;
	void addDependentSerie(Serie *serie);

	inline const OutsideSerieGroup *serieGroup() const { return m_serieGroup; }
	void addToSerieGroup(OutsideSerieGroup *group);

	inline LineType lineType() const  { return m_lineType; }
	void setLineType(LineType line_type);

	inline std::function<QString (const ValueChange &)> legendValueFormatter() const { return m_legendValueFormatter; }
	void setLegendValueFormatter(std::function<QString (const ValueChange &)> formatter);

	inline std::function<ValueChange::ValueY (const ValueChange &)> valueFormatter() const { return m_valueFormatter; }
	void setValueFormatter(std::function<ValueChange::ValueY (const ValueChange &)> formatter);

	inline double boolValue() const  { return m_boolValue; }
	void setBoolValue(double value);

	inline bool isHidden() const  { return !m_show; }
	void show(bool enable = true);
	void hide();

	inline bool isShowCurrent() const { return m_showCurrent; }
	void setShowCurrent(bool show);

	const SerieData &serieModelData(const View *view) const;
	const SerieData &serieModelData(const GraphModel *model) const;
	inline int serieIndex() const { return m_serieIndex; }

	void update(bool force = false);
	const Serie *masterSerie() const;
	View *view() const;

private:
	QString m_name;
	ValueType m_type;
	QColor m_color;
	QVector<BackgroundStripe*> m_backgroundStripes;
	int m_lineWidth = 1;
	YAxis m_relatedAxis = YAxis::Y1;
	QVector<Serie*> m_dependentSeries;
	OutsideSerieGroup *m_serieGroup = nullptr;
	LineType m_lineType = LineType::TwoDimensional;
	std::function<ValueChange::ValueY (const ValueChange &)> m_valueFormatter;
	std::function<QString (const ValueChange &)> m_legendValueFormatter;
	double m_boolValue = 0.0;
	bool m_show = true;
	bool m_showCurrent = true;
	int m_serieIndex = -1;
	QVector<QMetaObject::Connection> m_connections;

	SerieData::const_iterator displayedDataBegin = shv::gui::SerieData::const_iterator();
	SerieData::const_iterator displayedDataEnd = shv::gui::SerieData::const_iterator();
friend class View;
};

}
}
}
