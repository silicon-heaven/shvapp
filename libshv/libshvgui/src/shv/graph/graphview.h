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
		std::function<ValueChange::ValueY (const ValueChange &)> valueFormatter;
		std::function<QString (const ValueChange &)> legendValueFormatter;
		const SerieData *dataPtr;
		SerieData::const_iterator displayedDataBegin;
		SerieData::const_iterator displayedDataEnd;
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

	struct XAxisInterval
	{
		ValueChange::ValueX start;
		ValueChange::ValueX end;
	};

	GraphView(QWidget *parent);

	Settings settings;
	void setModelData(const GraphModel &model_data);
	void releaseModelData();

	void showRange(qint64 from, qint64 to);
	void zoom(qint64 center, double scale);

	Serie &addSerie(const Serie &serie);

	void splitSeries();
	void unsplitSeries();
	void showDependentSeries(bool enable);

	QVector<XAxisInterval> selections() const;
	XAxisInterval loadedRange() const;
	void addSelection(XAxisInterval selection);
	void clearSelections();

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
		qint64 start;
		qint64 end;

		bool containsValue(qint64 value) const;
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
	void paintSerie(QPainter *painter, const QRect &rect, int x_axis_position, const Serie &serie, qint64 min, qint64 max, const QPen &pen, bool fill_rect);
	void paintBoolSerie(QPainter *painter, const QRect &area, int x_axis_position, const Serie &serie, qint64 min, qint64 max, const QPen &pen, bool fill_rect);
	void paintValueSerie(QPainter *painter, const QRect &area, int x_axis_position, const Serie &serie, qint64 min, qint64 max, const QPen &pen, bool fill_rect);
	void paintSelections(QPainter *painter, const GraphArea &area);
	void paintSelection(QPainter *painter, const GraphArea &area, const Selection &selection, const QColor &color);
	void paintSerieList(QPainter *painter);
	void paintCrossLine(QPainter *painter, const GraphArea &area);
	void paintLegend(QPainter *painter);
	void paintCurrentPosition(QPainter *painter, const GraphArea &area);
	void paintCurrentPosition(QPainter *painter, const GraphArea &area, const Serie &serie, qint64 current);

	QString legend(qint64 position) const;
	QString legendRow(const Serie &serie, qint64 position) const;

	qint64 widgetPositionToXValue(int pos) const;
	qint64 rectPositionToXValue(int pos) const;
	int xValueToRectPosition(qint64 value) const;
	int xValueToWidgetPosition(qint64 value) const;
	const Selection *selectionOnValue(qint64 value) const;
	void updateLastValueInLastSelection(qint64 value);
	bool posInGraph(const QPoint &pos) const;
	bool posInRangeSelector(const QPoint &pos) const;
	void computeGeometry();
	bool hasVisibleSeries() const;
	int computeYLabelWidth(const Settings::Axis &axis, int &shownDecimalPoints) const;
	void computeRangeSelectorPosition();

	qint64 xValue(const ValueChange &value_change) const;
	qint64 xValue(const ValueChange::ValueX &value_x) const;
	ValueChange::ValueX internalToValueX(qint64 value) const;
	QString xValueString(qint64 value, const QString &datetime_format) const;
	void computeRange(double &min, double &max, const Serie &serie) const;
	void computeRange(int &min, int &max, const Serie &serie) const;
	void computeRange(qint64 &min, qint64 &max, const Serie &serie) const;
	void computeRange(double &min, double &max) const;
	void computeRange(int &min, int &max) const;
	void computeRange(qint64 &min, qint64 &max) const;
	void computeDataRange();
	SerieData::const_iterator findMinYValue(const SerieData::const_iterator &data_begin, const SerieData::const_iterator &data_end, qint64 x_value) const;
	SerieData::const_iterator findMaxYValue(const SerieData::const_iterator &data_begin, const SerieData::const_iterator &data_end, qint64 x_value) const;
//	template<typename T> static void mergeSerieMemberWithDefault(Serie &merged_serie, const Serie &param, T Serie::*member);

	void onModelDataChanged();

	const GraphModel *m_data;

	QList<GraphArea> m_graphArea;
	QRect m_xAxisDescriptionRect;
	QRect m_xAxisLabelRect;
	QRect m_serieListRect;
	QRect m_rangeSelectorRect;

	qint64 m_displayedRangeMin;
	qint64 m_displayedRangeMax;
	qint64 m_loadedRangeMin;
	qint64 m_loadedRangeMax;
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
