#pragma once

#include "../../shvguiglobal.h"

#include <QMap>
#include <QPushButton>
#include <QTimeZone>
#include <QTimer>
#include <QWidget>

#include "graphmodel.h"

#include <functional>

namespace shv {
namespace gui {
namespace graphview {

class Serie;
class OutsideSerieGroup;
class PointOfInterest;
class BackgroundStripe;

class RangeSelectorHandle : public QPushButton
{
	Q_OBJECT

public:
	RangeSelectorHandle(QWidget *parent);

protected:
	void paintEvent(QPaintEvent *event);
};

class SHVGUI_DECL_EXPORT View : public QWidget
{
	Q_OBJECT

	using SerieData = shv::gui::SerieData;

public:

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
		bool showBackgroundStripes = false;
		QTimeZone sourceDataTimeZone = QTimeZone::utc();
		QTimeZone viewTimeZone = QTimeZone::utc();
	};

	struct XAxisInterval
	{
		ValueChange::ValueX start;
		ValueChange::ValueX end;
	};

	View(QWidget *parent);
	~View();

	Settings settings;
	void setModel(GraphModel *model);
	void releaseModel();

	void showRange(qint64 from, qint64 to);
	void zoom(qint64 center, double scale);

	GraphModel *model() const;
	void addSerie(Serie *serie);
	Serie *serie(int index);
	inline const QList<Serie*> &series() const  { return m_series; }

	void splitSeries();
	void unsplitSeries();
	void showDependentSeries(bool enable);
	void computeGeometry();  //temporarily, before api rework

	QVector<XAxisInterval> selections() const;
	XAxisInterval loadedRange() const;
	void addSelection(XAxisInterval selection);
	void clearSelections();

	void addPointOfInterest(ValueChange::ValueX position, const QString &comment, const QColor &color);
	void addPointOfInterest(PointOfInterest *poi);
	void removePointsOfInterest();
	void showBackgroundStripes(bool enable);

	OutsideSerieGroup *addOutsideSerieGroup(const QString &name);
	void addOutsideSerieGroup(OutsideSerieGroup *group);

	void setViewTimezone(const QTimeZone &tz);
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
		QList<QRect> outsideSerieGroupsRects;
		int xAxisPosition;
		int x2AxisPosition;
		bool showYAxis;
		bool showY2Axis;
		bool switchAxes;
		QVector<const Serie*> series;
	};

	struct Selection
	{
		qint64 start;
		qint64 end;

		bool containsValue(qint64 value) const;
	};

	class SerieInGroup
	{
	public:
		const Serie *serie;
		const Serie *masterSerie;
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
	void paintSerie(QPainter *painter, const QRect &rect, int x_axis_position, const Serie *serie, qint64 min, qint64 max, const QPen &pen, bool fill_rect);
	void paintBoolSerie(QPainter *painter, const QRect &area, int x_axis_position, const Serie *serie, qint64 min, qint64 max, const QPen &pen, bool fill_rect);
	void paintBoolSerieAtPosition(QPainter *painter, const QRect &area, int y_position, const Serie *serie, qint64 min, qint64 max, bool fill_rect);
	void paintValueSerie(QPainter *painter, const QRect &area, int x_axis_position, const Serie *serie, qint64 min, qint64 max, const QPen &pen, bool fill_rect);
	void paintSelections(QPainter *painter, const GraphArea &area);
	void paintSelection(QPainter *painter, const GraphArea &area, const Selection &selection, const QColor &color);
	void paintSerieList(QPainter *painter);
	void paintCrossLine(QPainter *painter, const GraphArea &area);
	void paintLegend(QPainter *painter);
	void paintCurrentPosition(QPainter *painter, const GraphArea &area);
	void paintCurrentPosition(QPainter *painter, const GraphArea &area, const Serie *serie, qint64 current);
	void paintPointsOfInterest(QPainter *painter, const GraphArea &area);
	void paintPointOfInterest(QPainter *painter, const GraphArea &area, PointOfInterest *poi);
	void paintPointOfInterestVertical(QPainter *painter, const GraphArea &area, PointOfInterest *poi);
	void paintPointOfInterestPoint(QPainter *painter, const GraphArea &area, PointOfInterest *poi);
	void paintBackgroundStripes(QPainter *painter, const GraphArea &area);
	void paintOutsideSeriesGroups(QPainter *painter, const GraphArea &area);

	QString legend(qint64 position) const;
	QString legendRow(const Serie *serie, qint64 position) const;

	qint64 widgetPositionToXValue(int pos) const;
	qint64 rectPositionToXValue(int pos) const;
	int xValueToRectPosition(qint64 value) const;
	int xValueToWidgetPosition(qint64 value) const;
	const Selection *selectionOnValue(qint64 value) const;
	void updateLastValueInLastSelection(qint64 value);
	bool posInGraph(const QPoint &pos) const;
	bool posInRangeSelector(const QPoint &pos) const;
	bool hasVisibleSeries() const;
	int computeYLabelWidth(const Settings::Axis &axis, int &shownDecimalPoints) const;
	void computeRangeSelectorPosition();
	QVector<SerieInGroup> shownSeriesInGroup(const OutsideSerieGroup &group, const QVector<const Serie *> &only_series) const;
	QVector<const OutsideSerieGroup *> groupsForSeries(const QVector<const Serie*> &series) const;

	qint64 xValue(const ValueChange &value_change) const;
	qint64 xValue(const ValueChange::ValueX &value_x) const;
	ValueChange::ValueX internalToValueX(qint64 value) const;
	QString xValueString(qint64 value, const QString &datetime_format) const;
	void computeRange(double &min, double &max, const Serie *serie) const;
	void computeRange(int &min, int &max, const Serie *serie) const;
	void computeRange(qint64 &min, qint64 &max, const Serie *serie) const;
	template<typename T> void computeRange(T &min, T &max) const;
	void computeDataRange();
	QPainterPath createPoiPath(int x, int y) const;
	shv::gui::SerieData::const_iterator findMinYValue(const SerieData::const_iterator &data_begin, const SerieData::const_iterator &data_end, qint64 x_value) const;
	shv::gui::SerieData::const_iterator findMaxYValue(const SerieData::const_iterator &data_begin, const SerieData::const_iterator &data_end, qint64 x_value) const;

	static ValueChange::ValueY formattedSerieValue(const Serie *serie, SerieData::const_iterator it);
	int yPosition(ValueChange::ValueY value, const Serie *serie, const GraphArea &area);

	void onModelDataChanged();
	void showToolTip();

	GraphModel *m_model = nullptr;

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
	QList<Serie*> m_series;
	QList<QRect> m_seriesListRect;
	double m_xValueScale;
	QVector<QVector<const Serie*>> m_serieBlocks;
	RangeSelectorHandle *m_leftRangeSelectorHandle;
	RangeSelectorHandle *m_rightRangeSelectorHandle;
	int m_leftRangeSelectorPosition;
	int m_rightRangeSelectorPosition;
	QTimer m_toolTipTimer;
	QPoint m_toolTipPosition;
	QVector<PointOfInterest*> m_pointsOfInterest;
	QMap<const PointOfInterest*, QPainterPath> m_poiPainterPaths;
	QVector<OutsideSerieGroup*> m_outsideSeriesGroups;
	QVector<QMetaObject::Connection> m_connections;
};

} //namespace graphview
}
}
