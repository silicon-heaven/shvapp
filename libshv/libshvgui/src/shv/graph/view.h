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
	enum class Mode { Static, Dynamic };

	struct Settings
	{
		struct Axis
		{
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
		bool showSerieBackgroundStripes = false;
		QTimeZone sourceDataTimeZone = QTimeZone::utc();
		QTimeZone viewTimeZone = QTimeZone::utc();
		std::function <void (QMenu*)> contextMenuExtend;
	};

	View(QWidget *parent);
	~View();

	Settings settings;
	void setModel(GraphModel *model);
	void releaseModel();

	void showRange(ValueChange::ValueX from, ValueChange::ValueX to);
	void showRange(ValueXInterval range);
	void zoom(qint64 center, double scale);

	GraphModel *model() const;
	void addSerie(Serie *serie);
	Serie *serie(int index);
	inline const QList<Serie*> &series() const  { return m_series; }

	void splitSeries();
	void unsplitSeries();
	void showDependentSeries(bool enable);
	void computeGeometry();

	std::vector<ValueXInterval> selections() const;
	ValueXInterval loadedRange() const;
	ValueXInterval shownRange() const;
	void addSelection(ValueXInterval selection);
	void clearSelections();

	inline Mode mode() const { return m_mode; }
	void setMode(Mode mode);

	inline ValueChange::ValueX dymanicModePrepend() const { return internalToValueX(m_dynamicModePrepend); }
	void setDynamicModePrepend(ValueChange::ValueX prepend);

	void addPointOfInterest(ValueChange::ValueX position, const QString &comment, const QColor &color);
	void addPointOfInterest(PointOfInterest *poi);
	void removePointsOfInterest();

	void addBackgroundStripe(BackgroundStripe *stripe);
	void showBackgroundSerieStripes(bool enable);

	OutsideSerieGroup *addOutsideSerieGroup(const QString &name);
	void addOutsideSerieGroup(OutsideSerieGroup *group);

	void setViewTimezone(const QTimeZone &tz);
	Q_SIGNAL void selectionsChanged();

	void setLoadedRange(const ValueChange::ValueX &min, const ValueChange::ValueX &max);

protected:
	void resizeEvent(QResizeEvent *resize_event) Q_DECL_OVERRIDE;
	void paintEvent(QPaintEvent *paint_event) Q_DECL_OVERRIDE;
	void wheelEvent(QWheelEvent *wheel_event) Q_DECL_OVERRIDE;
	void mouseDoubleClickEvent(QMouseEvent *mouse_event) Q_DECL_OVERRIDE;
	void mousePressEvent(QMouseEvent *mouse_event) Q_DECL_OVERRIDE;
	void mouseMoveEvent(QMouseEvent *mouse_event) Q_DECL_OVERRIDE;
	void mouseReleaseEvent(QMouseEvent *mouse_event) Q_DECL_OVERRIDE;
	bool eventFilter(QObject *watched, QEvent *event) Q_DECL_OVERRIDE;

	virtual void onModelDataChanged();

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

		Selection(qint64 s = 0, qint64 e = 0) : start(s), end(e) {}

		Selection normalized() const {return Selection(qMin(start, end), qMax(start, end));}
		Selection& normalize() {if(end < start) qSwap(start, end); return *this;}
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
	void paintSerieBackgroundStripes(QPainter *painter, const GraphArea &area);
	void paintSerieVerticalBackgroundStripe(QPainter *painter, const GraphArea &area, const Serie *serie, const BackgroundStripe *stripe);
	void paintSerieHorizontalBackgroundStripe(QPainter *painter, const GraphArea &area, const Serie *serie, const BackgroundStripe *stripe);
	void paintViewBackgroundStripes(QPainter *painter, const GraphArea &area);
	void paintOutsideSeriesGroups(QPainter *painter, const GraphArea &area);

	QString legend(qint64 position) const;
	QString legendRow(const Serie *serie, qint64 position) const;

	int graphWidth() const;
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
	template<typename T> void computeRange(T &min, T &max) const;
	void computeDataRange();
	void showRangeInternal(qint64 from, qint64 to);
	QPainterPath createPoiPath(int x, int y) const;
	shv::gui::SerieData::const_iterator findMinYValue(const SerieData::const_iterator &data_begin, const SerieData::const_iterator &data_end, qint64 x_value) const;
	shv::gui::SerieData::const_iterator findMaxYValue(const SerieData::const_iterator &data_begin, const SerieData::const_iterator &data_end, qint64 x_value) const;

	static ValueChange::ValueY formattedSerieValue(const Serie *serie, SerieData::const_iterator it);
	int yPosition(ValueChange::ValueY value, const Serie *serie, const GraphArea &area);
	void unionLastSelection();

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
	qint64 m_dataRangeMin;
	qint64 m_dataRangeMax;
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
	QVector<BackgroundStripe*> m_backgroundStripes;
	QVector<QMetaObject::Connection> m_connections;
	Mode m_mode;
	qint64 m_dynamicModePrepend;
};

} //namespace graphview
}
}
