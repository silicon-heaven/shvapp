#pragma once

#include "../../shvguiglobal.h"

#include <QColor>
#include <QMap>
#include <QPainter>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

#include "graphmodel.h"

#include <functional>

namespace shv {
namespace gui {

class GraphView;
class Serie;

class RangeSelectorHandle : public QPushButton
{
	Q_OBJECT

public:
	RangeSelectorHandle(QWidget *parent);

protected:
	void paintEvent(QPaintEvent *event);
};

class SHVGUI_DECL_EXPORT BackgroundStripe : public QObject
{
	Q_OBJECT

public:
	BackgroundStripe(QObject *parent = 0);
	BackgroundStripe(ValueChange::ValueY min, ValueChange::ValueY max, QObject *parent = 0);

	inline const ValueChange::ValueY &min() const { return m_min; }
	inline const ValueChange::ValueY &max() const { return m_max; }

	void setMin(const ValueChange::ValueY &min);
	void setMax(const ValueChange::ValueY &max);
	void setRange(const ValueChange::ValueY &min, const ValueChange::ValueY &max);

private:
	ValueChange::ValueY m_min = 0;
	ValueChange::ValueY m_max = 0;
};

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

class SHVGUI_DECL_EXPORT Serie : public QObject
{
	Q_OBJECT

public:
	enum class LineType { OneDimensional, TwoDimensional };
	enum class YAxis { Y1, Y2 };

	Serie(ValueType type, int m_serieIndex, const QString &name, const QColor &color, QObject *parent = 0);
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
	void show();
	void hide();

	inline bool isShowCurrent() const { return m_showCurrent; }
	void setShowCurrent(bool show);

	const SerieData &serieModelData(const GraphView *view) const;
	const SerieData &serieModelData(const GraphModel *model) const;

private:
	void update();
	const Serie *masterSerie() const;
	GraphView *view() const;

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
friend class GraphView;
};

class SHVGUI_DECL_EXPORT GraphView : public QWidget
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
	};

	struct XAxisInterval
	{
		ValueChange::ValueX start;
		ValueChange::ValueX end;
	};

	GraphView(QWidget *parent);
	~GraphView();

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
	QVector<SerieInGroup> shownSeriesInGroup(const OutsideSerieGroup &group, const QVector<Serie *> &only_series) const;
	QVector<const OutsideSerieGroup *> groupsForSeries(const QVector<Serie*> &series) const;

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
	QVector<PointOfInterest*> m_pointsOfInterest;
	QMap<const PointOfInterest*, QPainterPath> m_poiPainterPaths;
	QVector<OutsideSerieGroup*> m_outsideSeriesGroups;
	QVector<QMetaObject::Connection> m_connections;
};

}
}
