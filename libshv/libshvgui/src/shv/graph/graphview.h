#pragma once

#include "../../shvguiglobal.h"

#include <QColor>
#include <QPainter>
#include <QPushButton>
#include <QTimer>
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

	using SerieData = shv::gui::SerieData;
public:
	class Serie;

	class BackgroundStripe
	{
	public:
		inline BackgroundStripe(ValueChange::ValueY min = 0, ValueChange::ValueY max = 0) : min(min), max(max) {}
		ValueChange::ValueY min = 0;
		ValueChange::ValueY max = 0;
	};

	struct OutsideSerieGroup
	{
		QString name;
		QVector<Serie*> series;
		int minimumHeight = 20;
		bool show = false;
		QColor backgroundColor;
		int spacing = 4;

	};
	struct Serie
	{
		enum class LineType { OneDimensional, TwoDimensional };
		enum class YAxis { Y1, Y2 };

		QString name;
		ValueType type;
		QColor color;
		YAxis relatedAxis;
		double boolValue = 0.0;
		bool show = true;
		bool showCurrent = true;
		int serieIndex = -1;
		std::function<ValueChange::ValueY (const ValueChange &)> valueFormatter = nullptr;
		std::function<QString (const ValueChange &)> legendValueFormatter = nullptr;
		SerieData::const_iterator displayedDataBegin = shv::gui::SerieData::const_iterator();
		SerieData::const_iterator displayedDataEnd = shv::gui::SerieData::const_iterator();
		QVector<Serie> dependentSeries = QVector<Serie>();
		QVector<BackgroundStripe> backgroundStripes = QVector<BackgroundStripe>();
		LineType lineType = LineType::TwoDimensional;
		OutsideSerieGroup *serieGroup = nullptr;
		int lineWidth = 1;

		//GraphView *m_view;

		//Serie() : modelIndex(-1), m_view(nullptr) {}
		//Serie(GraphView *view) : m_view(view) {}
		const SerieData &serieModelData(const GraphView *view) const;
		const SerieData &serieModelData(const GraphModel *model) const;
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
		bool showBackgroundStripes = false;
	};

	struct XAxisInterval
	{
		ValueChange::ValueX start;
		ValueChange::ValueX end;
	};

	GraphView(QWidget *parent);

	Settings settings;
	void setModel(GraphModel *model);
	void releaseModel();

	void showRange(qint64 from, qint64 to);
	void zoom(qint64 center, double scale);

	GraphModel *model() const;
	Serie &addSerie(const Serie &serie);
	Serie &serie(int index);
	inline const QList<Serie> &series() const  { return m_series; }

	void splitSeries();
	void unsplitSeries();
	void showDependentSeries(bool enable);

	QVector<XAxisInterval> selections() const;
	XAxisInterval loadedRange() const;
	void addSelection(XAxisInterval selection);
	void clearSelections();

	void addPointOfInterest(ValueChange::ValueX position, const QString &comment, const QColor &color);
	void removePointsOfInterest();
	void showBackgroundStripes(bool enable);

	OutsideSerieGroup &addOutsideSerieGroup(const QString &name);

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
		QVector<Serie*> series;
	};

	struct Selection
	{
		qint64 start;
		qint64 end;

		bool containsValue(qint64 value) const;
	};

	class PointOfInterest
	{
	public:
		qint64 position;
		QString comment;
		QColor color;
		QPainterPath painterPath;
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
	void paintSerie(QPainter *painter, const QRect &rect, int x_axis_position, const Serie &serie, qint64 min, qint64 max, const QPen &pen, bool fill_rect);
	void paintBoolSerie(QPainter *painter, const QRect &area, int x_axis_position, const Serie &serie, qint64 min, qint64 max, const QPen &pen, bool fill_rect);
	void paintBoolSerieAtPosition(QPainter *painter, const QRect &area, int y_position, const Serie &serie, qint64 min, qint64 max, bool fill_rect);
	void paintValueSerie(QPainter *painter, const QRect &area, int x_axis_position, const Serie &serie, qint64 min, qint64 max, const QPen &pen, bool fill_rect);
	void paintSelections(QPainter *painter, const GraphArea &area);
	void paintSelection(QPainter *painter, const GraphArea &area, const Selection &selection, const QColor &color);
	void paintSerieList(QPainter *painter);
	void paintCrossLine(QPainter *painter, const GraphArea &area);
	void paintLegend(QPainter *painter);
	void paintCurrentPosition(QPainter *painter, const GraphArea &area);
	void paintCurrentPosition(QPainter *painter, const GraphArea &area, const Serie &serie, qint64 current);
	void paintPointsOfInterest(QPainter *painter, const GraphArea &area);
	void paintBackgroundStripes(QPainter *painter, const GraphArea &area);
	void paintOutsideSeriesGroups(QPainter *painter, const GraphArea &area);

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
	QVector<SerieInGroup> shownSeriesInGroup(const OutsideSerieGroup &group, const QVector<Serie *> &only_series) const;
	QVector<OutsideSerieGroup *> groupsForSeries(const QVector<Serie*> &series) const;

	qint64 xValue(const ValueChange &value_change) const;
	qint64 xValue(const ValueChange::ValueX &value_x) const;
	ValueChange::ValueX internalToValueX(qint64 value) const;
	QString xValueString(qint64 value, const QString &datetime_format) const;
	void computeRange(double &min, double &max, const Serie &serie) const;
	void computeRange(int &min, int &max, const Serie &serie) const;
	void computeRange(qint64 &min, qint64 &max, const Serie &serie) const;
	template<typename T> void computeRange(T &min, T &max) const;
	void computeDataRange();
	QPainterPath createPoiPath(int x, int y) const;
	shv::gui::SerieData::const_iterator findMinYValue(const SerieData::const_iterator &data_begin, const SerieData::const_iterator &data_end, qint64 x_value) const;
	shv::gui::SerieData::const_iterator findMaxYValue(const SerieData::const_iterator &data_begin, const SerieData::const_iterator &data_end, qint64 x_value) const;

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
	QList<Serie> m_series;
	QList<bool> m_showSeries;
	QList<QRect> m_seriesListRect;
	double m_xValueScale;
	QVector<QVector<Serie*>> m_serieBlocks;
	RangeSelectorHandle *m_leftRangeSelectorHandle;
	RangeSelectorHandle *m_rightRangeSelectorHandle;
	int m_leftRangeSelectorPosition;
	int m_rightRangeSelectorPosition;
	QTimer m_toolTipTimer;
	QPoint m_toolTipPosition;
	QVector<PointOfInterest> m_pointsOfInterest;
	QVector<OutsideSerieGroup> m_outsideSeriesGroups;
};

}
}
