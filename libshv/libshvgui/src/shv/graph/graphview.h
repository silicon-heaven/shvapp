#pragma once

#include "../../shvguiglobal.h"

#include <QColor>
#include <QPushButton>
#include <QWidget>

#include "graphmodel.h"

#include <functional>

namespace shv {
namespace gui {

class RangeSelectorHandle : public QPushButton
{
	Q_OBJECT

public:
	RangeSelectorHandle(QWidget *parent);

protected:
	void paintEvent(QPaintEvent *event);
};

class SHVGUI_DECL_EXPORT GraphView : public QWidget
{
	Q_OBJECT

public:
	struct Serie
	{
		enum class YAxis { Y1, Y2 };

		QString name;
		ValueType type;
		QColor color;
		YAxis relatedAxis;
		double boolValue;
		bool show;
		bool showCurrent;
		int serieIndex;
		std::function<ValueChange (const ValueChange &)> valueFormatter;
		std::function<QString (const ValueChange &)> legendValueFormatter;
		const SerieData *dataPtr;
		QVector<Serie> dependentSeries;
	};

	struct Settings
	{
		struct Axis
		{
//			enum RangeType { Automatic, Fixed };
			enum RangeType { Fixed };
			bool show = true;
			QString description;
			QColor color;
			QFont descriptionFont;
			QFont labelFont;
			int lineWidth = 2;
			RangeType rangeType = Fixed;
			double rangeMin = 0.0;
			double rangeMax = 100.0;
		};

		struct RangeSelector
		{
			bool show = false;
			int height = 40;
			QColor color = Qt::gray;
			int lineWidth = 2;
		};

		struct SerieList
		{
			bool show = true;
			int height = 40;
			QFont font;
		};

		struct Grid
		{
			enum Type { FixedDistance, FixedCount };
			bool show = true;
			Type type = FixedCount;
			QColor color = QColor(Qt::lightGray);
			int fixedWidth = 100;
			int fixedCount = 8;
		};

		struct Margin
		{
			int left = 15;
			int top = 15;
			int right = 15;
			int bottom = 15;
		};

		struct Legend
		{
//			enum Type { Fixed, ToolTip };
			enum Type { ToolTip };

			bool show = true;
			Type type = ToolTip;
		};

		ValueType xAxisType = ValueType::TimeStamp;
		Axis xAxis;
		Axis yAxis;
		Axis y2Axis;
		RangeSelector rangeSelector;
		SerieList serieList;
		Grid verticalGrid;
		Grid horizontalGrid;
		Margin margin;
		QColor backgroundColor;
		QColor frameColor;
		Legend legend;
		QString legendStyle;
		bool showCrossLine = true;
		bool showDependent = true;
		bool enableOvelapingSelections = false;
	};

	struct ValueSelection
	{
		ValueChange start;
		ValueChange end;
	};

	GraphView(QWidget *parent);

	Settings settings;
	void setModelData(const GraphModel &model_data);

	void showRange(quint64 from, quint64 to);
	void zoom(quint64 center, double scale);

	Serie &addSerie(const Serie &serie);

	void splitSeries();
	void unsplitSeries();
	void showDependentSeries(bool enable);

	QVector<ValueSelection> selections() const; //only valueX filled in ValueChange!

	Q_SIGNAL void selectionsChanged();

protected:
	void resizeEvent(QResizeEvent *resize_event);
	void paintEvent(QPaintEvent *paint_event);
	void wheelEvent(QWheelEvent *wheel_event);
	void mouseDoubleClickEvent(QMouseEvent *mouse_event);
	void mousePressEvent(QMouseEvent *mouse_event);
	void mouseMoveEvent(QMouseEvent *mouse_event);
	void mouseReleaseEvent(QMouseEvent *mouse_event);
	bool eventFilter(QObject *watched, QEvent *event);

private:
	class GraphArea
	{
	public:
		QRect graphRect;
		QRect yAxisDescriptionRect;
		QRect y2AxisDescriptionRect;
		QRect yAxisLabelRect;
		QRect y2AxisLabelRect;
		int xAxisPosition;
		bool showYAxis;
		bool showY2Axis;
		bool switchAxes;
		QVector<Serie*> series;
	};

	struct Selection
	{
		quint64 start;
		quint64 end;

		bool containsValue(quint64 value) const;
	};

	void popupContextMenu(const QPoint &pos);
	void paintXAxisDescription(QPainter *painter);
	void paintYAxisDescription(QPainter *painter, const GraphArea &area);
	void paintY2AxisDescription(QPainter *painter, const GraphArea &area);
	void paintXAxis(QPainter *painter, const GraphArea &area);
	void paintYAxis(QPainter *painter, const GraphArea &area);
	void paintY2Axis(QPainter *painter, const GraphArea &area);
	void paintXAxisLabels(QPainter *painter);
	void paintYAxisLabels(QPainter *painter, const GraphArea &area);
	void paintY2AxisLabels(QPainter *painter, const GraphArea &area);
	void paintYAxisLabels(QPainter *painter, const Settings::Axis &axis, int shownDecimalPoints, const QRect &rect, int align);
	void paintVerticalGrid(QPainter *painter, const GraphArea &area);
	void paintHorizontalGrid(QPainter *painter, const GraphArea &area);
	void paintRangeSelector(QPainter *painter);
	void paintSeries(QPainter *painter, const GraphArea &area);
	void paintSerie(QPainter *painter, const QRect &rect, int x_axis_position, const Serie &serie, quint64 min, quint64 max, const QPen &pen, bool fill_rect);
	void paintBoolSerie(QPainter *painter, const QRect &area, int x_axis_position, const Serie &serie, quint64 min, quint64 max, const QPen &pen, bool fill_rect);
	void paintValueSerie(QPainter *painter, const QRect &area, int x_axis_position, const Serie &serie, quint64 min, quint64 max, const QPen &pen, bool fill_rect);
	void paintSelections(QPainter *painter, const GraphArea &area);
	void paintSelection(QPainter *painter, const GraphArea &area, const Selection &selection, const QColor &color);
	void paintSerieList(QPainter *painter);
	void paintCrossLine(QPainter *painter, const GraphArea &area);
	void paintLegend(QPainter *painter);
	void paintCurrentPosition(QPainter *painter, const GraphArea &area);
	void paintCurrentPosition(QPainter *painter, const GraphArea &area, const Serie &serie, quint64 current);

	QString legend(quint64 position);
	QString legendRow(const Serie &serie, quint64 position);

	quint64 widgetPositionToXValue(int pos);
	quint64 rectPositionToXValue(int pos);
	int xValueToRectPosition(quint64 value);
	int xValueToWidgetPosition(quint64 value);
	const Selection *selectionOnValue(quint64 value) const;
	void updateLastValueInLastSelection(quint64 value);
	bool posInGraph(const QPoint &pos);
	bool posInRangeSelector(const QPoint &pos);
	void computeGeometry();
	bool hasVisibleSeries() const;
	int computeYLabelWidth(const Settings::Axis &axis, int &shownDecimalPoints);
	void computeRangeSelectorPosition();

	quint64 xValue(const ValueChange &value_change) const;
	QString xValueString(quint64 value, const QString &datetime_format) const;
	void computeRange(double &min, double &max, const Serie &serie);
	void computeRange(int &min, int &max, const Serie &serie);
	void computeRange(quint64 &min, quint64 &max, const Serie &serie);
	void computeRange(double &min, double &max);
	void computeRange(int &min, int &max);
	void computeRange(quint64 &min, quint64 &max);
	SerieData::const_iterator findMinYValue(const SerieData &data, quint64 x_value) const;
//	template<typename T> static void mergeSerieMemberWithDefault(Serie &merged_serie, const Serie &param, T Serie::*member);

	void onModelDataChanged();

	const GraphModel *m_data;

	QList<GraphArea> m_graphArea;
	QRect m_xAxisDescriptionRect;
	QRect m_xAxisLabelRect;
	QRect m_serieListRect;
	QRect m_rangeSelectorRect;

	quint64 m_displayedRangeMin;
	quint64 m_displayedRangeMax;
	quint64 m_loadedRangeMin;
	quint64 m_loadedRangeMax;
	Selection m_zoomSelection;
	Qt::KeyboardModifiers m_currentSelectionModifiers;
	QList<Selection> m_selections;
	int m_moveStart;
	int m_currentPosition;
	double m_verticalGridDistance;
	double m_horizontalGridDistance;
	int m_yAxisShownDecimalPoints;
	int m_y2AxisShownDecimalPoints;
	QList<Serie> m_series;
	QList<bool> m_showSeries;
	QList<QRect> m_seriesListRect;
	double m_xValueScale;
	QVector<QVector<Serie*>> m_serieBlocks;
	RangeSelectorHandle *m_leftRangeSelectorHandle;
	RangeSelectorHandle *m_rightRangeSelectorHandle;
	int m_leftRangeSelectorPosition;
	int m_rightRangeSelectorPosition;
};

}
}
