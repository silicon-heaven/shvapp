#include "graphview.h"
#include <QDateTime>
#include <QDebug>
#include <QFrame>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

#include "float.h"

namespace shv {
namespace gui {

GraphView::GraphView(QWidget *parent) : QWidget(parent)
  , m_data(0)
  , m_displayedRangeMin(0LL)
  , m_displayedRangeMax(0LL)
  , m_loadedRangeMin(0LL)
  , m_loadedRangeMax(0LL)
  , m_zoomSelection({ 0, 0 })
  , m_currentSelectionModifiers(Qt::NoModifier)
  , m_moveStart(-1)
  , m_currentPosition(-1)
  , m_verticalGridDistance(0.0)
  , m_horizontalGridDistance(0.0)
  , m_yAxisShownDecimalPoints(0)
  , m_y2AxisShownDecimalPoints(0)
  , m_xValueScale(0.0)
  , m_leftRangeSelectorHandle(0)
  , m_rightRangeSelectorHandle(0)
  , m_leftRangeSelectorPosition(0)
  , m_rightRangeSelectorPosition(0)
{
	QColor text_color = palette().text().color();
	settings.xAxis.color = text_color;
	settings.yAxis.color = text_color;
	settings.y2Axis.color = text_color;
	QFont fd = font();
	fd.setBold(true);
//	fd.setPointSize(fd.pointSize() + 2);
	settings.xAxis.descriptionFont = fd;
	settings.yAxis.descriptionFont = fd;
	settings.y2Axis.descriptionFont = fd;
	QFont fl = font();
//	fd.setPointSize(fd.pointSize() + 1);
	settings.xAxis.labelFont = fl;
	settings.yAxis.labelFont = fl;
	settings.y2Axis.labelFont = fl;
	settings.xAxis.description = QStringLiteral("Time");
	settings.y2Axis.show = false;
	QFont fsl = font();
	fsl.setBold(true);
	settings.serieList.font = fsl;
	settings.backgroundColor = palette().base().color();
	settings.frameColor = palette().mid().color();
	settings.legendStyle =
			"<style>"
			"  table {"
			"    width: 100%;"
			"  }"
			"  td.label {"
			"    padding-right: 3px; "
			"    padding-left: 5px;"
			"    text-align: right;"
			"  }"
			"  td.value {"
			"    font-weight: bold;"
			"    padding-right: 5px;"
			"    padding-left: 3px;"
			"  }"
			"  table.head td {"
			"    padding-top: 5px;"
			"  }"
			"  td.headLabel {"
			"    padding-left: 5px;"
			"    padding-right: 5px;"
			"  }"
			"  td.headValue {"
			"    font-weight: bold;"
			"    padding-right: 5px;"
			"    padding-left: 3px;"
			"  }"
			"</style>";
//	setFocus();
	setMouseTracking(true);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_leftRangeSelectorHandle = new RangeSelectorHandle(this);
	m_leftRangeSelectorHandle->installEventFilter(this);
	m_leftRangeSelectorHandle->hide();
	m_rightRangeSelectorHandle = new RangeSelectorHandle(this);
	m_rightRangeSelectorHandle->installEventFilter(this);
	m_rightRangeSelectorHandle->hide();
}

GraphView::~GraphView()
{
	cleanSeries();
}

void GraphView::cleanSerie(Serie &serie)
{
	if (serie.formattedDataPtr){
		delete serie.formattedDataPtr;
		serie.formattedDataPtr = 0;
	}
	serie.dataPtr = 0;
}

void GraphView::cleanSeries()
{
	for (Serie &serie : m_series) {
		cleanSerie(serie);
		for (Serie &dep_serie : serie.dependentSeries) {
			cleanSerie(dep_serie);
		}
	}
}

void GraphView::acquireSerieData(Serie &serie)
{
	if (serie.valueFormatter) {
		serie.formattedDataPtr = new SerieData(settings.xAxisType, serie.type);
		for (const ValueChange &value : *m_data->serieData(serie.serieIndex)) {
			serie.formattedDataPtr->push_back(serie.valueFormatter(value));
		}
		serie.dataPtr = serie.formattedDataPtr;
	}
	else {
		serie.dataPtr = m_data->serieData(serie.serieIndex);
	}
}

void GraphView::setModelData(const GraphModel &model_data)
{
	cleanSeries();

	m_data = &model_data;

	m_loadedRangeMin = UINT64_MAX;
	m_loadedRangeMax = 0;
	for (Serie &serie : m_series) {
		acquireSerieData(serie);
		for (Serie &dep_serie : serie.dependentSeries) {
			acquireSerieData(dep_serie);
		}
	}
	switch (settings.xAxisType) {
	case ValueType::Double:
	{
		double min, max;
		computeRange(min, max);
		if (min < 0.0) {
			throw std::runtime_error("GraphView cannot operate negative values on x axis");
		}
		m_xValueScale = INT64_MAX / max;
		m_displayedRangeMin = m_loadedRangeMin = min * m_xValueScale;
		m_displayedRangeMax = m_loadedRangeMax = max * m_xValueScale;
		break;
	}
	case ValueType::Int:
	{
		int min, max;
		computeRange(min, max);
		if (min < 0.0) {
			throw std::runtime_error("GraphView cannot operate negative values on x axis");
		}
		m_displayedRangeMin = m_loadedRangeMin = min;
		m_displayedRangeMax = m_loadedRangeMax = max;
		break;
	}
	case ValueType::TimeStamp:
	{
		computeRange(m_loadedRangeMin, m_loadedRangeMax);
		m_displayedRangeMin = m_loadedRangeMin;
		m_displayedRangeMax = m_loadedRangeMax;
		break;
	}
	default:
		break;
	}
	if (settings.rangeSelector.show) {
		m_leftRangeSelectorHandle->show();
		m_rightRangeSelectorHandle->show();
	}
	computeGeometry();
	repaint();
}

void GraphView::resizeEvent(QResizeEvent *resize_event)
{
	QWidget::resizeEvent(resize_event);
	if (!m_data || !m_series.count()) {
		return;
	}

	computeGeometry();
	repaint();
}

int GraphView::computeYLabelWidth(const Settings::Axis &axis, int &shownDecimalPoints)
{
	double range = axis.rangeMax - axis.rangeMin;
	int place_value = 0;
	while (range > 1.0) {
		++place_value;
		range /= 10.0;
	}
	shownDecimalPoints = 0;
	if (place_value < 2) {
		range = axis.rangeMax - axis.rangeMin;
		while (range < 1.0 || (place_value + shownDecimalPoints) < 3) {
			++shownDecimalPoints;
			range *= 10.0;
		}
	}
	QString test_string;
	test_string.fill('8', place_value + shownDecimalPoints + (shownDecimalPoints ? 1 : 0) + (axis.rangeMin < 0 ? 1 : 0));
	return QFontMetrics(axis.labelFont).width(test_string);
}

void GraphView::computeRangeSelectorPosition()
{
	if (m_loadedRangeMax > m_loadedRangeMin) {
		m_leftRangeSelectorPosition = m_rangeSelectorRect.x() +
									  m_rangeSelectorRect.width()  * (m_displayedRangeMin - m_loadedRangeMin) / (m_loadedRangeMax - m_loadedRangeMin);
		m_rightRangeSelectorPosition = m_rangeSelectorRect.x() +
									   m_rangeSelectorRect.width() * (m_displayedRangeMax - m_loadedRangeMin) / (m_loadedRangeMax - m_loadedRangeMin);
	}
	else {
		m_leftRangeSelectorPosition = m_rangeSelectorRect.x();
		m_rightRangeSelectorPosition = m_rangeSelectorRect.x() + m_rangeSelectorRect.width();
	}
	m_leftRangeSelectorHandle->move(
				m_leftRangeSelectorPosition - (m_leftRangeSelectorHandle->width() / 2),
				m_rangeSelectorRect.y() + (m_rangeSelectorRect.height() - m_leftRangeSelectorHandle->height()) / 2
				);
	m_rightRangeSelectorHandle->move(
				m_rightRangeSelectorPosition - (m_rightRangeSelectorHandle->width() / 2),
				m_rangeSelectorRect.y() + (m_rangeSelectorRect.height() - m_rightRangeSelectorHandle->height()) / 2
				);
}

void GraphView::computeGeometry()
{
	QRect all_graphs_rect(settings.margin.left, settings.margin.top,
					 width() - settings.margin.left - settings.margin.right,
					 height() - settings.margin.top - settings.margin.bottom);
	if (settings.serieList.show) {
		m_serieListRect = QRect(all_graphs_rect.x(), all_graphs_rect.bottom() - settings.serieList.height, all_graphs_rect.width(), settings.serieList.height);
		all_graphs_rect.setBottom(all_graphs_rect.bottom() - settings.serieList.height);
	}
	if (settings.rangeSelector.show) {
		m_rangeSelectorRect = QRect(all_graphs_rect.x(), all_graphs_rect.bottom() - settings.rangeSelector.height, all_graphs_rect.width(), settings.rangeSelector.height);
		all_graphs_rect.setBottom(all_graphs_rect.bottom() - settings.rangeSelector.height - 15);
	}
	if (settings.xAxis.show) {
		if (!settings.xAxis.description.isEmpty()) {
			int x_axis_description_font_height = QFontMetrics(settings.xAxis.descriptionFont).lineSpacing() * 1.5;
			m_xAxisDescriptionRect = QRect(all_graphs_rect.left(), all_graphs_rect.bottom() - x_axis_description_font_height, all_graphs_rect.width(), x_axis_description_font_height);
			all_graphs_rect.setBottom(all_graphs_rect.bottom() - x_axis_description_font_height);
		}
		int x_axis_label_font_height = QFontMetrics(settings.xAxis.labelFont).lineSpacing();
		m_xAxisLabelRect = QRect(all_graphs_rect.left(), all_graphs_rect.bottom() - x_axis_label_font_height, all_graphs_rect.width(), x_axis_label_font_height);
		all_graphs_rect.setBottom(all_graphs_rect.bottom() - x_axis_label_font_height * 1.4);
	}


	int vertical_space = 20;
	QVector<QVector<Serie*>> visible_blocks;

	for (int i = 0; i < m_serieBlocks.count(); ++i) {
		for (const Serie *serie : m_serieBlocks[i]) {
			if (serie->show) {
				visible_blocks << m_serieBlocks[i];
				break;
			}
		}
	}

	if (visible_blocks.count() == 0 && m_graphArea.count()) { //pokud nemame zobrazenou zadnou serii, pocitame geometrii
		visible_blocks << m_graphArea[0].series;              //podle naposledy zobrazene
	}
	int graph_count = visible_blocks.count();

	if (graph_count) {
		m_graphArea.clear();

		int single_graph_height = (all_graphs_rect.height() - (vertical_space * (graph_count - 1))) / graph_count;

		int max_y_description_width = 0;
		int max_y_label_width = 0;
		int max_y2_description_width = 0;
		int max_y2_label_width = 0;

		for (int i = 0; i < visible_blocks.count(); ++i) {
			int y_description_width = 0;
			int y_label_width = 0;
			int y2_description_width = 0;
			int y2_label_width = 0;

			GraphArea area;
			area.series = visible_blocks[i];

			area.showYAxis = false;
			area.showY2Axis = false;
			for (const Serie *serie : area.series) {
				if (serie->relatedAxis == Serie::YAxis::Y1) {
					area.showYAxis = true;
				}
				else if (serie->relatedAxis == Serie::YAxis::Y2) {
					area.showY2Axis = true;
				}
			}
			area.switchAxes = (area.showY2Axis && !area.showYAxis);

			area.graphRect = QRect(all_graphs_rect.x(),
								   all_graphs_rect.y() + (i * (vertical_space + single_graph_height)),
								   all_graphs_rect.width(),
								   single_graph_height);

			if (settings.yAxis.show && (area.showYAxis || (area.showY2Axis && area.switchAxes))) {
				if (area.showYAxis) {
					if (!settings.yAxis.description.isEmpty()) {
						y_description_width = QFontMetrics(settings.yAxis.descriptionFont).lineSpacing() * 1.5;
					}
					y_label_width = computeYLabelWidth(settings.yAxis, m_yAxisShownDecimalPoints);
				}
				else {
					if (!settings.y2Axis.description.isEmpty()) {
						y_description_width = QFontMetrics(settings.y2Axis.descriptionFont).lineSpacing() * 1.5;
					}
					y_label_width = computeYLabelWidth(settings.y2Axis, m_y2AxisShownDecimalPoints);
				}
			}

			if (settings.y2Axis.show && area.showY2Axis && !area.switchAxes) {
				if (!settings.y2Axis.description.isEmpty()) {
					y2_description_width = QFontMetrics(settings.y2Axis.descriptionFont).lineSpacing() * 1.5;
				}
				y2_label_width = computeYLabelWidth(settings.y2Axis, m_y2AxisShownDecimalPoints);
			}

			double x_scale = (double)(settings.yAxis.rangeMax - settings.yAxis.rangeMin) / area.graphRect.height();
			area.xAxisPosition = area.graphRect.y() + settings.yAxis.rangeMax / x_scale;

			m_graphArea << area;

			max_y_description_width = qMax(max_y_description_width, y_description_width);
			max_y_label_width = qMax(max_y_label_width, y_label_width);
			max_y2_description_width = qMax(max_y2_description_width, y2_description_width);
			max_y2_label_width = qMax(max_y2_label_width, y2_label_width);
		}
		int left_offset = max_y_description_width + max_y_label_width;
		int right_offset = max_y2_description_width + max_y2_label_width;
		if (left_offset) {
			left_offset += 5;
			if (settings.xAxis.show) {
				m_xAxisDescriptionRect.setX(m_xAxisDescriptionRect.x() + left_offset);
			}
			if (settings.serieList.show) {
				m_serieListRect.setX(m_serieListRect.x() + left_offset);
			}
			if (settings.rangeSelector.show) {
				m_rangeSelectorRect.setX(m_rangeSelectorRect.x() + left_offset);
			}
			for (GraphArea &area : m_graphArea) {
				area.yAxisDescriptionRect = QRect(area.graphRect.x(), area.graphRect.y(), max_y_description_width, area.graphRect.height());
				area.yAxisLabelRect = QRect(area.graphRect.x() + max_y_description_width, area.graphRect.y(), max_y_label_width, area.graphRect.height());
				area.graphRect.setX(area.graphRect.x() + left_offset);
			}
		}
		if (right_offset) {
			right_offset += 5;
			if (settings.xAxis.show) {
				m_xAxisDescriptionRect.setRight(m_xAxisDescriptionRect.right() - right_offset);
			}
			if (settings.serieList.show) {
				m_serieListRect.setRight(m_serieListRect.right() - right_offset);
			}
			if (settings.rangeSelector.show) {
				m_rangeSelectorRect.setRight(m_rangeSelectorRect.right() - right_offset);
			}
			for (GraphArea &area : m_graphArea) {
				area.y2AxisDescriptionRect = QRect(area.graphRect.right() - max_y2_description_width, area.graphRect.y(), max_y2_description_width, area.graphRect.height());
				area.y2AxisLabelRect = QRect(area.graphRect.right() - max_y2_description_width - max_y2_label_width, area.graphRect.y(), max_y2_label_width, area.graphRect.height());
				area.graphRect.setRight(area.graphRect.right() - right_offset);
			}
		}
	}
	else {
		GraphArea area;

		area.showYAxis = false;
		area.showY2Axis = false;
		area.switchAxes = false;

		area.graphRect = all_graphs_rect;
		area.xAxisPosition = area.graphRect.bottom();

		m_graphArea << area;
	}

	if (settings.verticalGrid.type == Settings::Grid::Type::FixedCount) {
		m_verticalGridDistance = (double)m_graphArea[0].graphRect.width() / (settings.verticalGrid.fixedCount + 1.0);
	}
	else if (settings.verticalGrid.type == Settings::Grid::Type::FixedDistance) {
		m_verticalGridDistance = settings.verticalGrid.fixedWidth;
	}

	if (settings.horizontalGrid.type == Settings::Grid::Type::FixedCount) {
		m_horizontalGridDistance = (double)m_graphArea[0].graphRect.height() / (settings.horizontalGrid.fixedCount + 1.0);
	}
	else if (settings.horizontalGrid.type == Settings::Grid::Type::FixedDistance) {
		m_horizontalGridDistance = settings.horizontalGrid.fixedWidth;
	}
	if (settings.rangeSelector.show) {
		computeRangeSelectorPosition();
	}
}

bool GraphView::hasVisibleSeries() const
{
	for (const Serie &serie : m_series) {
		if (serie.show) {
			return true;
		}
	}
	return false;
}

void GraphView::paintEvent(QPaintEvent *paint_event)
{
	QWidget::paintEvent(paint_event);

	QPainter painter(this);
	painter.fillRect(0, 0, width() - 1, height() - 1, settings.backgroundColor); //??
	painter.save();
	painter.setPen(settings.frameColor);
	painter.drawRect(0, 0, width() - 1, height() - 1);  //??
	painter.restore();

	if (!m_data) {
		return;
	}

	QRect paint_rect = paint_event->rect();

	if (settings.serieList.show && paint_rect.intersects(m_serieListRect)) {
		paintSerieList(&painter);
	}
	if (settings.rangeSelector.show && paint_rect.intersects(m_rangeSelectorRect)) {
		paintRangeSelector(&painter);
	}
	if (settings.xAxis.show) {
		if (!settings.xAxis.description.isEmpty() && paint_rect.intersects(m_xAxisDescriptionRect)) {
			paintXAxisDescription(&painter);
		}
		if (paint_rect.intersects(m_xAxisLabelRect)) {
			paintXAxisLabels(&painter);
		}
	}
//    if (settings.legend.show && settings.legend.type == Settings::Legend::Type::Fixed) {
//        paintLegend(&painter);
//    }
	for (const GraphArea &area : m_graphArea) {

		if (settings.yAxis.show && (area.showYAxis || (area.showY2Axis && area.switchAxes))) {
			if (((area.showYAxis && !settings.yAxis.description.isEmpty()) || (area.showY2Axis && !settings.y2Axis.description.isEmpty())) && paint_rect.intersects(area.yAxisDescriptionRect)) {
				paintYAxisDescription(&painter, area);
			}
			if (paint_rect.intersects(area.yAxisLabelRect)) {
				paintYAxisLabels(&painter, area);
			}
		}

		if (settings.y2Axis.show && area.showY2Axis && !area.switchAxes) {
			if (!settings.y2Axis.description.isEmpty() && paint_rect.intersects(area.y2AxisDescriptionRect)) {
				paintY2AxisDescription(&painter, area);
			}
			if (paint_rect.intersects(area.y2AxisLabelRect)) {
				paintY2AxisLabels(&painter, area);
			}
		}

		if (paint_rect.intersects(area.graphRect)) {
			if (settings.xAxis.show) {
				paintXAxis(&painter, area);
			}
			if (settings.yAxis.show && (area.showYAxis || (area.showY2Axis && area.switchAxes))) {
				paintYAxis(&painter, area);
			}
			if (settings.y2Axis.show && area.showY2Axis && !area.switchAxes) {
				paintY2Axis(&painter, area);
			}
			if (settings.verticalGrid.show) {
				paintVerticalGrid(&painter, area);
			}
			if (settings.horizontalGrid.show) {
				paintHorizontalGrid(&painter, area);
			}
			if (m_series.count()) {
				paintSeries(&painter, area);
			}
			if (m_selections.count() || m_zoomSelection.start || m_zoomSelection.end) {
				paintSelections(&painter, area);
			}
			if (m_currentPosition != -1) {
				if (settings.showCrossLine) {
					paintCrossLine(&painter, area);
				}
				paintCurrentPosition(&painter, area);
			}
		}
	}
}

bool GraphView::posInGraph(const QPoint &pos)
{
	QRect graph_rect;
	if (m_graphArea.count()) {
		graph_rect = m_graphArea[0].graphRect;
	}
	for (int i = 1; i < m_graphArea.count(); ++i) {
		graph_rect = graph_rect.united(m_graphArea[i].graphRect);
	}
	return graph_rect.contains(pos, true);
}

bool GraphView::posInRangeSelector(const QPoint &pos)
{
	return m_rangeSelectorRect.contains(pos);
}

void GraphView::wheelEvent(QWheelEvent *wheel_event)
{
	if (posInGraph(wheel_event->pos())) {

		quint64 center = widgetPositionToXValue(wheel_event->pos().x());
		if (wheel_event->angleDelta().y() > 0) {
			zoom(center, 6.5 / 10.0);
		}
		else if (wheel_event->angleDelta().y() < 0) {
			zoom(center, 10.0 / 6.5);
		}
	}
}

void GraphView::mouseDoubleClickEvent(QMouseEvent *mouse_event)
{
	QPoint mouse_pos = mouse_event->pos();
	if (posInGraph(mouse_pos) || posInRangeSelector(mouse_pos)) {
		if (m_zoomSelection.start || m_zoomSelection.end) {
			m_zoomSelection = { 0, 0 };
			m_currentSelectionModifiers = Qt::NoModifier;
		}
		if (m_loadedRangeMin != m_displayedRangeMin || m_loadedRangeMax != m_displayedRangeMax) {
			showRange(m_loadedRangeMin, m_loadedRangeMax);
		}
	}
}

void GraphView::mousePressEvent(QMouseEvent *mouse_event)
{
	QPoint pos = mouse_event->pos();
	if (mouse_event->buttons() & Qt::LeftButton) {
		if (posInGraph(pos)) {
			if (mouse_event->modifiers() & Qt::ControlModifier || mouse_event->modifiers() & Qt::ShiftModifier) {
				quint64 value = widgetPositionToXValue(pos.x());
				m_currentSelectionModifiers = mouse_event->modifiers();
				if (mouse_event->modifiers() & Qt::ControlModifier) {
					m_zoomSelection = { value, value };
				}
				else if (settings.enableOvelapingSelections || !selectionOnValue(value)){
					m_selections << Selection { value, value };
				}
			}
			else if (mouse_event->modifiers() == Qt::NoModifier) {
				m_moveStart = pos.x() - m_graphArea[0].graphRect.x();
			}
		}
		else if (posInRangeSelector(pos)) {
			if (mouse_event->modifiers() == Qt::NoModifier) {
				if (pos.x() >= m_leftRangeSelectorPosition && pos.x() <= m_rightRangeSelectorPosition) {
					m_moveStart = pos.x() - m_rangeSelectorRect.x();
				}
			}
		}
		else {
			for (int i = 0; i < m_seriesListRect.count(); ++i) {
				const QRect &rect = m_seriesListRect[i];
				if (rect.contains(pos)) {
					m_series[i].show = !m_series[i].show;
					computeGeometry();
					repaint();
					break;
				}
			}
		}
	}
	else if (mouse_event->buttons() & Qt::RightButton) {
		if (posInGraph(pos) && mouse_event->modifiers() == Qt::NoModifier) {
			popupContextMenu(pos);
		}
	}
}

const GraphView::Selection *GraphView::selectionOnValue(quint64 value) const
{
	for (const Selection &selection : m_selections) {
		if (selection.containsValue(value)) {
			return &selection;
		}
	}
	return 0;
}

void GraphView::updateLastValueInLastSelection(quint64 value)
{
	Selection &last_selection = m_selections.last();
	const Selection *overlaping_selection = selectionOnValue(value);
	if (overlaping_selection && overlaping_selection != &last_selection && !settings.enableOvelapingSelections) {
		if (last_selection.start <= last_selection.end) {
			last_selection.end = overlaping_selection->start;
		}
		else {
			last_selection.end = overlaping_selection->end;
		}
	}
	else {
		last_selection.end = value;
	}
	Q_EMIT selectionsChanged();
}

void GraphView::mouseMoveEvent(QMouseEvent *mouse_event)
{
	QPoint pos = mouse_event->pos();
	if (posInGraph(pos)) {
		int x_pos = pos.x() - m_graphArea[0].graphRect.x();
		if (mouse_event->buttons() & Qt::LeftButton) {
			m_currentPosition = -1;
			if (m_currentSelectionModifiers != Qt::NoModifier) {
				quint64 value = widgetPositionToXValue(pos.x());
				if (m_currentSelectionModifiers & Qt::ControlModifier) {
					m_zoomSelection.end = value;
				}
				else {
					updateLastValueInLastSelection(value);
				}
				repaint();
			}
			else if (mouse_event->modifiers() == Qt::NoModifier) {
				if (m_moveStart != -1) {
					qint64 difference = (m_displayedRangeMax - m_displayedRangeMin) * ((double)(x_pos - m_moveStart) / m_graphArea[0].graphRect.width());
					if (difference < 0LL) {
						if ((qint64)(m_loadedRangeMax - m_displayedRangeMax) < -difference) {
							difference = -(m_loadedRangeMax - m_displayedRangeMax);
						}
					}
					else {
						if ((qint64)(m_displayedRangeMin - m_loadedRangeMin) < difference) {
							difference = m_displayedRangeMin - m_loadedRangeMin;
						}
					}
					showRange(m_displayedRangeMin - difference, m_displayedRangeMax - difference);
					m_moveStart = x_pos;
				}
			}
		}
		else if (mouse_event->buttons() == Qt::NoButton) {
			if (m_currentPosition != x_pos) {
				m_currentPosition = x_pos;
				repaint();
			}
			if (settings.legend.show && settings.legend.type == Settings::Legend::Type::ToolTip && hasVisibleSeries()) {
				QPoint mouse_pos = mouse_event->pos();
				QPoint top_left = mouse_pos;
				top_left.setY(top_left.y() - 5);
				QPoint bottom_right = mouse_pos;
				bottom_right.setY(bottom_right.y() + 5);
				QToolTip::showText(mouse_event->globalPos(), legend(rectPositionToXValue(x_pos)), this, QRect(top_left, bottom_right));
			}
		}
	}
	else if (posInRangeSelector(pos)) {
		int x_pos = pos.x() - m_rangeSelectorRect.x();
		if (mouse_event->buttons() & Qt::LeftButton) {
			if (mouse_event->modifiers() == Qt::NoModifier) {
				if (m_moveStart != -1) {
					qint64 difference = (qint64)(m_loadedRangeMax - m_loadedRangeMin) * (m_moveStart - x_pos) / m_rangeSelectorRect.width();
					if (difference < 0LL) {
						if ((qint64)(m_loadedRangeMax - m_displayedRangeMax) < -difference) {
							difference = -(m_loadedRangeMax - m_displayedRangeMax);
						}
					}
					else {
						if ((qint64)(m_displayedRangeMin - m_loadedRangeMin) < difference) {
							difference = m_displayedRangeMin - m_loadedRangeMin;
						}
					}
					showRange(m_displayedRangeMin - difference, m_displayedRangeMax - difference);
					m_moveStart = x_pos;
				}
			}
		}
	}
	else {
		bool on_serie_list = false;
		for (const QRect &rect : m_seriesListRect) {
			if (rect.contains(pos)) {
				on_serie_list = true;
				break;
			}
		}
		Qt::CursorShape current_cursor = cursor().shape();
		Qt::CursorShape requested_cursor;
		if (on_serie_list) {
			requested_cursor = Qt::PointingHandCursor;
		}
		else {
			requested_cursor = Qt::ArrowCursor;
		}
		if (current_cursor != requested_cursor) {
			setCursor(requested_cursor);
		}
	}
}

void GraphView::mouseReleaseEvent(QMouseEvent *mouse_event)
{
	QPoint pos = mouse_event->pos();
	if (posInGraph(pos)) {
		if (mouse_event->button() == Qt::LeftButton) {
			if (m_currentSelectionModifiers != Qt::NoModifier) {
				quint64 value = widgetPositionToXValue(pos.x());
				if (m_currentSelectionModifiers & Qt::ControlModifier) {
					quint64 start = m_zoomSelection.start;
					quint64 end = value;
					if (end < start) {
						start = end;
						end = m_zoomSelection.start;
					}
					if (start != end) {
						showRange(start, end);
					}
					m_zoomSelection = { 0, 0 };
				}
				else if (m_currentSelectionModifiers & Qt::ShiftModifier) {
					updateLastValueInLastSelection(value);
				}
				m_currentSelectionModifiers = Qt::NoModifier;
			}
			else if (m_moveStart != -1) {
				m_moveStart = -1;
			}
		}
	}
	else if (posInRangeSelector(pos)) {
		if (m_moveStart != -1) {
			m_moveStart = -1;
		}
	}
}

bool GraphView::eventFilter(QObject *watched, QEvent *event)
{
	auto rangeRectPositionToXValue = [this](int pos)->quint64
	{
		return (pos - m_rangeSelectorRect.x()) * (m_loadedRangeMax - m_loadedRangeMin) / m_rangeSelectorRect.width();
	};

	static int last_mouse_position = 0;
	if (watched == m_leftRangeSelectorHandle || watched == m_rightRangeSelectorHandle) {
		if (event->type() == QEvent::MouseButtonPress) {
			QMouseEvent *mouse_event = (QMouseEvent *)event;
			last_mouse_position = mouse_event->pos().x();
		}
		else if (event->type() == QEvent::MouseMove) {
			QMouseEvent *mouse_event = (QMouseEvent *)event;
			if (watched == m_leftRangeSelectorHandle) {
				int new_position = m_leftRangeSelectorPosition + mouse_event->pos().x() - last_mouse_position;
				if (new_position >= m_rangeSelectorRect.x() && new_position + 1 < m_rightRangeSelectorPosition) {
					if (new_position == m_rangeSelectorRect.x()) {
						m_displayedRangeMin = m_loadedRangeMin;
					}
					else {
						m_displayedRangeMin = rangeRectPositionToXValue(new_position);
					}
					computeGeometry();
					repaint();
				}
			}
			else { //(watched == m_rightRangeSelectorHandle)
				int new_position = m_rightRangeSelectorPosition + mouse_event->pos().x() - last_mouse_position;
				if (new_position <= m_rangeSelectorRect.right() && new_position - 1 > m_leftRangeSelectorPosition) {
					if (new_position == m_rangeSelectorRect.right()) {
						m_displayedRangeMax = m_loadedRangeMax;
					}
					else {
						m_displayedRangeMax = rangeRectPositionToXValue(new_position);
					}
					computeGeometry();
					repaint();
				}

			}
			return true;
		}
	}
	return false;
}

void GraphView::popupContextMenu(const QPoint &pos)
{
	QMenu popup_menu(this);
	QAction *zoom_to_fit = popup_menu.addAction(tr("Zoom to &fit"), [this]() {
		showRange(m_loadedRangeMin, m_loadedRangeMax);
	});
	if (m_displayedRangeMin == m_loadedRangeMin && m_displayedRangeMax == m_loadedRangeMax) {
		zoom_to_fit->setEnabled(false);
	}
	quint64 matching_value = widgetPositionToXValue(pos.x());

	for (int i = 0; i < m_selections.count(); ++i) {
		const Selection &selection = m_selections[i];
		if (selection.containsValue(matching_value)) {
			QAction *zoom_to_selection = popup_menu.addAction(tr("&Zoom to selection"), [this, i]() {
				showRange(m_selections[i].start, m_selections[i].end);
			});
			if (m_selections[i].start == m_displayedRangeMin && m_selections[i].end == m_displayedRangeMax) {
				zoom_to_selection->setEnabled(false);
			}
			popup_menu.addAction(tr("&Remove selection"), [this, i]() {
				m_selections.removeAt(i);
				repaint();
				Q_EMIT selectionsChanged();
			});
			break;
		}
	}
	QAction *remove_all_selections = popup_menu.addAction(tr("Remove &all selections"), [this]() {
		m_selections.clear();
		repaint();
	});
	if (m_selections.count() == 0) {
		remove_all_selections->setEnabled(false);
	}
	popup_menu.exec(mapToGlobal(pos));
}

void GraphView::zoom(quint64 center, double scale)
{
	if (scale == 1.0) {
		return;
	}
	quint64 old_length = m_displayedRangeMax - m_displayedRangeMin;
	quint64 new_length = old_length * scale;
	quint64 from = 0LL;
	quint64 to = 0LL;
	if (new_length > old_length) {
		quint64 difference = new_length - old_length;
		quint64 difference_from = difference * (double)(center - m_displayedRangeMin) / old_length;
		quint64 difference_to = difference - difference_from;
		if (m_displayedRangeMin - m_loadedRangeMin > difference_from) {
			from = m_displayedRangeMin - difference_from;
		}
		else {
			from = m_loadedRangeMin;
		}
		if (m_loadedRangeMax - m_displayedRangeMax > difference_to) {
			to = m_displayedRangeMax + difference_to;
		}
		else {
			to = m_loadedRangeMax;
		}
	}
	else if (new_length < old_length) {
		quint64 difference = old_length - new_length;
		quint64 difference_from = difference * (double)(center - m_displayedRangeMin) / old_length;
		quint64 difference_to = difference - difference_from;
		from = m_displayedRangeMin;
		if (from + difference_from < center) {
			from += difference_from;
		}
		to = m_displayedRangeMax;
		if (to - difference_to > center) {
			to -= difference_to;
		}
	}

	showRange(from, to);
}

//template<typename T>
//void GraphView::mergeSerieMemberWithDefault(Serie &merged_serie, const Serie &param, T Serie::*member)
//{
//	if (m_defaultSerie.*member != param.*member) {
//		merged_serie.*member = param.*member;
//	}
//}

GraphView::Serie &GraphView::addSerie(const Serie &serie)
{
	if (serie.dataPtr) {
		throw std::runtime_error("Data ptr in serie must not be set as param");
	}
	if (serie.type == ValueType::Bool && !serie.boolValue) {
		throw std::runtime_error("Bool serie must have set boolValue");
	}
//	Serie merged_serie = m_defaultSerie;
//	if (!serie.name.isEmpty()) {
//		merged_serie.name = serie.name;
//	}
//	else {
//		merged_serie.name = "Serie " + QString::number(m_series.count() + 1);
//	}

//	mergeSerieMemberWithDefault(merged_serie, serie, &Serie::type);
//	mergeSerieMemberWithDefault(merged_serie, serie, &Serie::color);
//	mergeSerieMemberWithDefault(merged_serie, serie, &Serie::relatedAxis);
//	mergeSerieMemberWithDefault(merged_serie, serie, &Serie::boolValue);
//	mergeSerieMemberWithDefault(merged_serie, serie, &Serie::show);
//	mergeSerieMemberWithDefault(merged_serie, serie, &Serie::showCurrent);

//	merged_serie.dataGetter = serie.dataGetter;
//	merged_serie.legendValueFormatter = serie.legendValueFormatter;

//	m_series.append(merged_serie);
	m_series.append(serie);
	if (m_serieBlocks.count() == 0) {
		m_serieBlocks.append(QVector<Serie*>());
	}
	m_serieBlocks.last() << &m_series.last();

	return m_series.last();
}

void GraphView::splitSeries()
{
	m_serieBlocks.clear();
	for (Serie &serie : m_series) {
		m_serieBlocks.append(QVector<Serie*>());
		m_serieBlocks.last() << &serie;
	}
	computeGeometry();
	repaint();
}

void GraphView::unsplitSeries()
{
	m_serieBlocks.clear();
	m_serieBlocks.append(QVector<Serie*>());
	for (Serie &serie : m_series) {
		m_serieBlocks.last() << &serie;
	}
	computeGeometry();
	repaint();
}

void GraphView::showDependentSeries(bool enable)
{
	settings.showDependent = enable;
	repaint();
}

QVector<GraphView::ValueSelection> GraphView::selections() const
{
	ValueChange start;
	ValueChange end;

	QVector<ValueSelection> selections;
	for (const Selection &selection : m_selections) {
		quint64 s_start = selection.start;
		quint64 s_end = selection.end;
		if (s_start > s_end) {
			std::swap(s_start, s_end);
		}
		switch (settings.xAxisType) {
		case ValueType::TimeStamp:
			start.valueX.timeStamp = s_start;
			end.valueX.timeStamp = s_end;
			break;
		case ValueType::Int:
			start.valueX.intValue = s_start;
			end.valueX.intValue = s_end;
			break;
		case ValueType::Double:
			start.valueX.doubleValue = s_start / m_xValueScale;
			end.valueX.doubleValue = s_end / m_xValueScale;
			break;
		default:
			break;
		}
		selections << ValueSelection { start, end };
	}

	return selections;
}

void GraphView::showRange(quint64 from, quint64 to)
{
	if (from < m_loadedRangeMin || from > to) {
		from = m_loadedRangeMin;
	}
	if (to > m_loadedRangeMax) {
		to = m_loadedRangeMax;
	}
	if (from == m_displayedRangeMin && to == m_displayedRangeMax) {
		return;
	}
	m_displayedRangeMin = from;
	m_displayedRangeMax = to;
	if (settings.rangeSelector.show) {
		computeRangeSelectorPosition();
	}
	repaint();
}

void GraphView::paintYAxisDescription(QPainter *painter, const GraphArea &area)
{
	const Settings::Axis &axis = area.switchAxes ? settings.y2Axis : settings.yAxis;
	painter->save();
	QPen pen(axis.color);
	painter->setPen(pen);
	painter->setFont(axis.descriptionFont);
	painter->translate(area.yAxisDescriptionRect.x(), area.yAxisDescriptionRect.bottom());
	painter->rotate(270.0);
	painter->drawText(QRect(0, 0, area.yAxisDescriptionRect.height(), area.yAxisDescriptionRect.width()), axis.description, QTextOption(Qt::AlignCenter));
	painter->restore();
}

void GraphView::paintY2AxisDescription(QPainter *painter, const GraphArea &area)
{
	painter->save();
	QPen pen(settings.y2Axis.color);
	painter->setPen(pen);
	painter->setFont(settings.y2Axis.descriptionFont);
	painter->translate(area.y2AxisDescriptionRect.right(), area.y2AxisDescriptionRect.y());
	painter->rotate(90.0);
	painter->drawText(QRect(0, 0, area.y2AxisDescriptionRect.height(), area.y2AxisDescriptionRect.width()), settings.y2Axis.description, QTextOption(Qt::AlignCenter));
	painter->restore();
}

void GraphView::paintYAxis(QPainter *painter, const GraphArea &area)
{
	painter->save();
	QPen pen(settings.yAxis.color);
	pen.setWidth(settings.yAxis.lineWidth);
	painter->setPen(pen);
	painter->drawLine(area.graphRect.topLeft(), area.graphRect.bottomLeft());
	painter->restore();
}

void GraphView::paintY2Axis(QPainter *painter, const GraphArea &area)
{
	painter->save();
	QPen pen(settings.y2Axis.color);
	pen.setWidth(settings.y2Axis.lineWidth);
	painter->setPen(pen);
	painter->drawLine(area.graphRect.topRight(), area.graphRect.bottomRight());
	painter->restore();
}

void GraphView::paintXAxisLabels(QPainter *painter)
{
	painter->save();
	QPen pen(settings.xAxis.color);
	painter->setPen(pen);
	painter->setFont(settings.xAxis.labelFont);
	QString time_format = "HH:mm:ss";
	if (m_displayedRangeMax - m_displayedRangeMin < 30000) {
		time_format += ".zzz";
	}
	int label_width = painter->fontMetrics().width(time_format);

	quint64 ts;
	QString x_value_label;

	const GraphArea &area = m_graphArea[0];
	for (double x = area.graphRect.x(); x < area.graphRect.right(); x += m_verticalGridDistance) {
		ts = widgetPositionToXValue(x);
		x_value_label = xValueString(ts, time_format);
		painter->drawText(x - label_width / 2, m_xAxisLabelRect.y(), label_width, m_xAxisLabelRect.height(), Qt::AlignCenter, x_value_label);
	}
	if (width() - area.graphRect.right()  > label_width / 2) {
		x_value_label = xValueString(m_displayedRangeMax, time_format);
		painter->drawText(area.graphRect.right() + (settings.margin.right / 2) - label_width, m_xAxisLabelRect.y(), label_width, m_xAxisLabelRect.height(), Qt::AlignCenter, x_value_label);
	}
	painter->restore();
}

void GraphView::paintYAxisLabels(QPainter *painter, const GraphView::GraphArea &area)
{
	if (!area.switchAxes) {
		paintYAxisLabels(painter, settings.yAxis, m_yAxisShownDecimalPoints, area.yAxisLabelRect, Qt::AlignVCenter | Qt::AlignRight);
	}
	else {
		paintYAxisLabels(painter, settings.y2Axis, m_y2AxisShownDecimalPoints, area.yAxisLabelRect, Qt::AlignVCenter | Qt::AlignRight);
	}
}

void GraphView::paintY2AxisLabels(QPainter *painter, const GraphArea &area)
{
	paintYAxisLabels(painter, settings.y2Axis, m_y2AxisShownDecimalPoints, area.y2AxisLabelRect, Qt::AlignVCenter | Qt::AlignLeft);
}

void GraphView::paintYAxisLabels(QPainter *painter, const Settings::Axis &axis, int shownDecimalPoints, const QRect &rect, int align)
{
	painter->save();
	QPen pen(axis.color);
	painter->setPen(pen);
	painter->setFont(axis.labelFont);
	int font_height = painter->fontMetrics().lineSpacing();

	double y_scale = (double)(axis.rangeMax - axis.rangeMin) / rect.height();

	double x_axis_position = rect.y() + axis.rangeMax / y_scale;

	QByteArray format;
	format = "%." + QByteArray::number(shownDecimalPoints) + 'f';
	for (double y = rect.top(); y <= rect.bottom() + 1; y += m_horizontalGridDistance) {
		QString value;
		value.sprintf(format.constData(), (x_axis_position - y) * y_scale);
		painter->drawText(rect.x(), (int)y - font_height / 2, rect.width(), font_height, align, value);
	}
	painter->restore();

}

void GraphView::paintXAxisDescription(QPainter *painter)
{
	painter->save();
	QPen pen(settings.xAxis.color);
	painter->setPen(pen);
	painter->setFont(settings.xAxis.descriptionFont);
	painter->drawText(m_xAxisDescriptionRect, settings.xAxis.description, QTextOption(Qt::AlignCenter));

	painter->restore();
}

void GraphView::paintXAxis(QPainter *painter, const GraphArea &area)
{
	painter->save();
	QPen pen(settings.xAxis.color);
	pen.setWidth(settings.xAxis.lineWidth);
	painter->setPen(pen);

	painter->drawLine(area.graphRect.x(), area.xAxisPosition, area.graphRect.right(), area.xAxisPosition);

	painter->restore();
}

void GraphView::paintVerticalGrid(QPainter *painter, const GraphArea &area)
{
	painter->save();
	painter->setPen(settings.verticalGrid.color);
	for (double x = area.graphRect.x() + m_verticalGridDistance; x < area.graphRect.right(); x += m_verticalGridDistance) {
		painter->drawLine(x, area.graphRect.y(), x, area.graphRect.bottom());
	}
	if (!settings.y2Axis.show) {
		painter->drawLine(area.graphRect.topRight(), area.graphRect.bottomRight());
	}
	painter->restore();
}

void GraphView::paintHorizontalGrid(QPainter *painter, const GraphArea &area)
{
	painter->save();
	painter->setPen(settings.horizontalGrid.color);
	for (double y = area.graphRect.bottom(); y > area.graphRect.y(); y -= m_horizontalGridDistance) {
		painter->drawLine(area.graphRect.x(), y, area.graphRect.right(), y);
	}
	painter->drawLine(area.graphRect.topLeft(), area.graphRect.topRight());
	painter->restore();
}

void GraphView::paintRangeSelector(QPainter *painter)
{
	painter->save();

	painter->translate(m_rangeSelectorRect.topLeft());

	for (int i = 0; i < m_series.count(); ++i) {
		const Serie &serie = m_series[i];
		if (serie.show) {
			int x_axis_position;
			if (serie.relatedAxis == Serie::YAxis::Y1) {
				double x_scale = (double)(settings.yAxis.rangeMax - settings.yAxis.rangeMin) / m_rangeSelectorRect.height();
				x_axis_position = settings.yAxis.rangeMax / x_scale;
			}
			else {
				double x_scale = (double)(settings.y2Axis.rangeMax - settings.y2Axis.rangeMin) / m_rangeSelectorRect.height();
				x_axis_position = settings.y2Axis.rangeMax / x_scale;
			}
			QPen pen(settings.rangeSelector.color);
			pen.setWidth(settings.rangeSelector.lineWidth);
			paintSerie(painter, m_rangeSelectorRect, x_axis_position, serie, m_loadedRangeMin, m_loadedRangeMax, pen, true);
			break;
		}
	}

	painter->restore();
	painter->save();

	QColor shade_color = settings.backgroundColor;
	shade_color.setAlpha(160);
	if (m_leftRangeSelectorPosition > m_rangeSelectorRect.x()) {
		painter->fillRect(QRect(m_rangeSelectorRect.topLeft(), QPoint(m_leftRangeSelectorPosition, m_rangeSelectorRect.bottom())), shade_color);
	}
	if (m_rightRangeSelectorPosition < m_rangeSelectorRect.right()) {
		painter->fillRect(QRect(QPoint(m_rightRangeSelectorPosition, m_rangeSelectorRect.top()), m_rangeSelectorRect.bottomRight()), shade_color);
	}
	QPen pen(Qt::gray);
	pen.setWidth(1);
	painter->setPen(pen);
	QRect selection(QPoint(m_leftRangeSelectorPosition, m_rangeSelectorRect.top()), QPoint(m_rightRangeSelectorPosition, m_rangeSelectorRect.bottom()));
	if (m_leftRangeSelectorPosition > m_rangeSelectorRect.x()) {
		painter->drawLine(m_rangeSelectorRect.topLeft(), m_rangeSelectorRect.bottomLeft());
		painter->drawLine(m_rangeSelectorRect.bottomLeft(), selection.bottomLeft());
	}
	if (m_rightRangeSelectorPosition < m_rangeSelectorRect.right()) {
		painter->drawLine(m_rangeSelectorRect.topRight(), m_rangeSelectorRect.bottomRight());
		painter->drawLine(selection.bottomRight(), m_rangeSelectorRect.bottomRight());
	}
	pen.setColor(Qt::black);
	pen.setWidth(2);
	painter->setPen(pen);
	if (m_leftRangeSelectorPosition > m_rangeSelectorRect.x()) {
		painter->drawLine(m_rangeSelectorRect.topLeft(), selection.topLeft());
	}
	painter->drawLine(selection.topLeft(), selection.bottomLeft());
	painter->drawLine(selection.bottomLeft(), selection.bottomRight());
	painter->drawLine(selection.topRight(), selection.bottomRight());
	if (m_rightRangeSelectorPosition < m_rangeSelectorRect.right()) {
		painter->drawLine(selection.topRight(), m_rangeSelectorRect.topRight());
	}

	painter->restore();
}

void GraphView::paintSeries(QPainter *painter, const GraphArea &area)
{
	painter->save();

	painter->translate(area.graphRect.topLeft());

	for (int i = 0; i < area.series.count(); ++i) {
		const Serie &serie = *area.series[i];
		int x_axis_position = area.xAxisPosition - area.graphRect.top();
		QPen pen(serie.color);
		if (serie.show && settings.showDependent) {
			for (const Serie &dependent_serie : serie.dependentSeries) {
				paintSerie(painter, area.graphRect, x_axis_position, dependent_serie, m_displayedRangeMin, m_displayedRangeMax, pen, false);
			}
		}
		paintSerie(painter, area.graphRect, x_axis_position, serie, m_displayedRangeMin, m_displayedRangeMax, pen, false);
	}
	painter->restore();
}

void GraphView::paintSerie(QPainter *painter, const QRect &rect, int x_axis_position, const Serie &serie, quint64 min, quint64 max, const QPen &pen, bool fill_rect)
{
	if (serie.show) {
		if (serie.type == ValueType::Bool) {
			paintBoolSerie(painter, rect, x_axis_position, serie, min, max, pen, fill_rect);
		}
		else {
			paintValueSerie(painter, rect, x_axis_position, serie, min, max, pen, fill_rect);
		}
	}
}

void GraphView::paintBoolSerie(QPainter *painter, const QRect &rect, int x_axis_position, const Serie &serie, quint64 min, quint64 max, const QPen &pen, bool fill_rect)
{
	const SerieData &data = *serie.dataPtr;
	if (data.size() == 0) {
		return;
	}

	double x_scale = (double)(max - min) / rect.width();

	double y_scale = 0.0;
	if (serie.relatedAxis == Serie::YAxis::Y1) {
		y_scale = (double)(settings.yAxis.rangeMax - settings.yAxis.rangeMin) / rect.height();
	}
	else if (serie.relatedAxis == Serie::YAxis::Y2) {
		y_scale = (double)(settings.y2Axis.rangeMax - settings.y2Axis.rangeMin) / rect.height();
	}
	painter->setPen(pen);

	auto begin = data.begin();
	if (min > m_loadedRangeMin) {
		begin = findMinYValue(data, min);
	}
	auto end = data.end();
	if (max < m_loadedRangeMax) {
		end = std::upper_bound(begin, data.end(), max, [this](quint64 value, const ValueChange &value_change) {
			return value < xValue(value_change);
		});
	}

	int y_true_line_position = x_axis_position - serie.boolValue / y_scale;

	QPolygon polygon;
	polygon << QPoint(0, rect.height());

	for (auto it = begin; it != end; ++it) {
		if (it->valueY.boolValue) {
			int begin_line = (xValue(*it) - min) / x_scale;
			if (begin_line < 0) {
				begin_line = 0;
			}
			int end_line;
			if (it + 1 == end) {
				end_line = rect.width();
			}
			else {
				++it;
				end_line = (xValue(*it) - min) / x_scale;
			}
			painter->drawLine(begin_line, y_true_line_position, end_line, y_true_line_position);
			if (fill_rect) {
				polygon << QPoint(begin_line, rect.height()) << QPoint(begin_line, y_true_line_position)
						<< QPoint(end_line, y_true_line_position) << QPoint(end_line, rect.height());
			}
		}
	}
	if (fill_rect) {
		polygon << QPoint(rect.width(), rect.height());
		QPainterPath path;
		path.addPolygon(polygon);
		path.closeSubpath();
		QRect bounding_rect = polygon.boundingRect();
		QLinearGradient gradient(bounding_rect.topLeft(), bounding_rect.bottomLeft());
		gradient.setColorAt(0, pen.color().lighter());
		gradient.setColorAt(1, pen.color());
		painter->fillPath(path, gradient);
	}
}

void GraphView::paintValueSerie(QPainter *painter, const QRect &rect, int x_axis_position, const Serie &serie, quint64 min, quint64 max, const QPen &pen, bool fill_rect)
{
	const SerieData &data = *serie.dataPtr;
	if (data.size() == 0) {
		return;
	}

	double x_scale = (double)(max - min) / rect.width();

	double y_scale = 0.0;
	if (serie.relatedAxis == Serie::YAxis::Y1) {
		y_scale = (double)(settings.yAxis.rangeMax - settings.yAxis.rangeMin) / rect.height();
	}
	else if (serie.relatedAxis == Serie::YAxis::Y2) {
		y_scale = (double)(settings.y2Axis.rangeMax - settings.y2Axis.rangeMin) / rect.height();
	}
	painter->setPen(pen);

	auto begin = data.begin();
	if (min > m_loadedRangeMin) {
		begin = findMinYValue(data, min);
	}
	auto end = data.end();
	if (max < m_loadedRangeMax) {
		end = std::upper_bound(begin, data.end(), max, [this](quint64 value, const ValueChange &value_change) {
			return value < xValue(value_change);
		});
	}

	QPoint first_point;
	if (serie.type == ValueType::Double) {
		first_point = QPoint(0, x_axis_position - (begin->valueY.doubleValue / y_scale));
	}
	else if (serie.type == ValueType::Int) {
		first_point = QPoint(0, x_axis_position - (begin->valueY.intValue / y_scale));
	}
	int max_on_first = first_point.y();
	int min_on_first = first_point.y();
	int last_on_first = first_point.y();

	QPolygon polygon;
	polygon << QPoint(0, rect.height());
	polygon << first_point;

	for (auto it = begin + 1; it != end; ++it) {
		const ValueChange &value_change = *it;
		quint64 x_value = xValue(value_change);
		QPoint last_point;
		if (serie.type == ValueType::Double) {
			double y_value = value_change.valueY.doubleValue;
			last_point = QPoint((x_value - min) / x_scale, x_axis_position - (y_value / y_scale));
		}
		else if (serie.type == ValueType::Int) {
			int y_value = value_change.valueY.intValue;
			last_point = QPoint((x_value - min) / x_scale, x_axis_position - (y_value / y_scale));
		}

		if (last_point.x() == first_point.x()) {
			if (max_on_first < last_point.y()) {
				max_on_first = last_point.y();
			}
			if (min_on_first > last_point.y()) {
				min_on_first = last_point.y();
			}
			last_on_first = last_point.y();
		}
		else { //if (last_point.x() > first_point.x()) {
			if (max_on_first > first_point.y() || min_on_first < first_point.y()) {
				painter->drawLine(first_point.x(), max_on_first, first_point.x(), min_on_first);
			}
			painter->drawLine(first_point.x(), last_on_first, last_point.x(), last_on_first);
			painter->drawLine(last_point.x(), last_on_first, last_point.x(), last_point.y());

			if (fill_rect) {
				polygon << QPoint(first_point.x(), last_on_first);
				polygon << QPoint(last_point.x(), last_on_first);
				polygon << last_point;
			}

			first_point = last_point;
			max_on_first = first_point.y();
			min_on_first = first_point.y();
			last_on_first = first_point.y();
		}
	}
	if (first_point.x() != rect.width() && max < m_loadedRangeMax) {
		painter->drawLine(first_point.x(), first_point.y(), rect.width(), first_point.y());
		polygon << QPoint(rect.width(), first_point.y());
	}
	if (fill_rect) {
		polygon << QPoint(rect.width(), rect.height());
		QPainterPath path;
		path.addPolygon(polygon);
		path.closeSubpath();
		QRect bounding_rect = polygon.boundingRect();
		QLinearGradient gradient(bounding_rect.topLeft(), bounding_rect.bottomLeft());
		gradient.setColorAt(0, pen.color().lighter());
		gradient.setColorAt(1, pen.color());
		painter->fillPath(path, gradient);
	}
}

void GraphView::paintSelection(QPainter *painter, const GraphArea &area, const Selection &selection, const QColor &color)
{
	painter->save();

	QColor fill_color = color;
	fill_color.setAlpha(60);
	QColor select_color = color;
	select_color.setAlpha(170);
	QPen pen;
	pen.setColor(select_color);
	pen.setWidth(2);
	painter->setPen(pen);

	quint64 start = selection.start;
	quint64 end = selection.end;
	bool draw_left_line = true;
	bool draw_right_line = true;
	if (start > end) {
		std::swap(start, end);
	}
	if (start < m_displayedRangeMax && end > m_displayedRangeMin) {
		if (start < m_displayedRangeMin) {
			start = m_displayedRangeMin;
			draw_left_line = false;
		}
		if (end > m_displayedRangeMax) {
			end = m_displayedRangeMax;
			draw_right_line = false;
		}
		int r_start = xValueToWidgetPosition(start);
		int r_end = xValueToWidgetPosition(end);
		if (draw_left_line) {
			painter->drawLine(r_start, area.graphRect.y(), r_start, area.graphRect.bottom());
		}
		if (draw_right_line) {
			painter->drawLine(r_end, area.graphRect.y(), r_end, area.graphRect.bottom());
		}
		painter->fillRect(r_start, area.graphRect.y(), r_end - r_start, area.graphRect.height(), fill_color);
	}

	painter->restore();
}

void GraphView::paintSelections(QPainter *painter, const GraphArea &area)
{
	for (const Selection &selection : m_selections) {
		paintSelection(painter, area, selection, palette().color(QPalette::Highlight));
	}
	paintSelection(painter, area, m_zoomSelection, Qt::green);
}

void GraphView::paintSerieList(QPainter *painter)
{
	painter->save();

	painter->setFont(settings.serieList.font);

	int space = 20;
	QList<int> widths;
	int total_width = 0;
	for (const Serie &serie : m_series) {
		int width = painter->fontMetrics().width(serie.name);
		total_width += width;
		widths << width;
	}
	int square_width = painter->fontMetrics().width("X");
	total_width += (widths.count() * 3 * square_width) + ((widths.count() - 1) * space);

	int x = (m_serieListRect.width() - total_width) / 2;
	if (x < 0) {
		x = 0;  //idealne rozdelit na 2 radky
	}
	m_seriesListRect.clear();
	int label_height = painter->fontMetrics().lineSpacing();
	for (int i = 0; i < m_series.count(); ++i) {
		const Serie &serie = m_series[i];
		if (serie.show) {
			painter->setPen(palette().color(QPalette::Active, QPalette::Text));
		}
		else {
			painter->setPen(palette().color(QPalette::Disabled, QPalette::Text));
		}
		m_seriesListRect << QRect(m_serieListRect.x() + x, m_serieListRect.top() + ((m_serieListRect.height() - label_height) / 2), 3 * square_width + widths[i], label_height);
		QRect square_rect(m_serieListRect.x() + x, m_serieListRect.top() + ((m_serieListRect.height() - square_width) / 2), square_width, square_width);
		painter->fillRect(square_rect, serie.show ? serie.color : serie.color.lighter(180));
		painter->drawRect(square_rect);
		x += 2 * square_width;
		painter->drawText(m_serieListRect.x() + x, m_serieListRect.top(), widths[i], m_serieListRect.height(), Qt::AlignVCenter | Qt::AlignLeft, serie.name);
		x += widths[i] + space;
	}
	painter->restore();
}

void GraphView::paintCrossLine(QPainter *painter, const GraphArea &area)
{
	if (!hasVisibleSeries()) {
		return;
	}
	painter->save();

	painter->setPen(Qt::DashLine);
	painter->drawLine(m_currentPosition + area.graphRect.x(), area.graphRect.top(), m_currentPosition + area.graphRect.x(), area.graphRect.bottom());

	painter->restore();
}

void GraphView::paintLegend(QPainter *painter)
{
	Q_UNUSED(painter)
}

void GraphView::paintCurrentPosition(QPainter *painter, const GraphArea &area, const Serie &serie, quint64 current)
{
	if (serie.showCurrent) {
		auto begin = findMinYValue(*serie.dataPtr, current);
		double range;
		if (serie.relatedAxis == Serie::YAxis::Y1) {
			range = settings.yAxis.rangeMax - settings.yAxis.rangeMin;
		}
		else {
			range = settings.y2Axis.rangeMax - settings.y2Axis.rangeMin;
		}
		double scale = range / area.graphRect.height();
		int y_position = 0;
		if (serie.type == ValueType::Double) {
			y_position = begin->valueY.doubleValue / scale;
		}
		else if (serie.type == ValueType::Int) {
			y_position = begin->valueY.intValue / scale;
		}
		else if (serie.type == ValueType::Bool) {
			y_position = begin->valueY.boolValue ? (serie.boolValue / scale) : 0;
		}
		QPainterPath path;
		path.addEllipse(m_currentPosition + area.graphRect.x() - 3, area.xAxisPosition - y_position - 3, 6, 6);
		painter->fillPath(path, serie.color);
	}

}

void GraphView::paintCurrentPosition(QPainter *painter, const GraphArea &area)
{
	painter->save();
	quint64 current = rectPositionToXValue(m_currentPosition);
	for (int i = 0; i < area.series.count(); ++i) {
		const Serie &serie = *area.series[i];
		if (serie.show && serie.dataPtr->size()) {
			paintCurrentPosition(painter, area, serie, current);
			if (settings.showDependent) {
				for (const Serie &dependent_serie : serie.dependentSeries) {
					if (dependent_serie.show) {
						paintCurrentPosition(painter, area, dependent_serie, current);
					}
				}
			}
		}
	}
	painter->restore();
}

QString GraphView::legendRow(const Serie &serie, quint64 position)
{
	QString s;
	if (serie.show) {
		auto begin = findMinYValue(*serie.dataPtr, position);
		s = s + "<tr><td class=\"label\">" + serie.name + ":</td><td class=\"value\">";
		if (serie.legendValueFormatter) {
			s += serie.legendValueFormatter(*begin);
		}
		else if (serie.type == ValueType::Double) {
			s += QString::number(begin->valueY.doubleValue);
		}
		else if (serie.type == ValueType::Int) {
			s += QString::number(begin->valueY.intValue);
		}
		else if (serie.type == ValueType::Bool) {
			s += begin->valueY.boolValue ? tr("true") : tr("false");
		}
		s += "</td></tr>";
	}
	return s;
}

QString GraphView::legend(quint64 position)
{
	QString s = "<html><head>" + settings.legendStyle + "</head><body>";
	s = s + "<table class=\"head\"><tr><td class=\"headLabel\">" + settings.xAxis.description + ":</td><td class=\"headValue\">" +
		xValueString(position, "dd.MM.yyyy HH.mm.ss.zzz").replace(" ", "&nbsp;") + "</td></tr></table><hr>";
	s += "<table>";
	for (const Serie &serie : m_series) {
		if (serie.dataPtr->size()) {
			s += legendRow(serie, position);
			if (serie.show && settings.showDependent) {
				for (const Serie &dependent_serie : serie.dependentSeries) {
					s += legendRow(dependent_serie, position);
				}
			}
		}
	}
	s += "</table></body></html>";
	return s;
}

quint64 GraphView::widgetPositionToXValue(int pos)
{
	return rectPositionToXValue(pos - m_graphArea[0].graphRect.x());
}

quint64 GraphView::rectPositionToXValue(int pos)
{
	return m_displayedRangeMin + ((m_displayedRangeMax - m_displayedRangeMin) * ((double)pos / m_graphArea[0].graphRect.width()));
}

int GraphView::xValueToRectPosition(quint64 value)
{
	return (value - m_displayedRangeMin) * m_graphArea[0].graphRect.width() / (m_displayedRangeMax - m_displayedRangeMin);
}

int GraphView::xValueToWidgetPosition(quint64 value)
{
	return m_graphArea[0].graphRect.x() + xValueToRectPosition(value);
}

quint64 GraphView::xValue(const ValueChange &value_change) const
{
	quint64 val;
	switch (settings.xAxisType) {
	case ValueType::TimeStamp:
		val = value_change.valueX.timeStamp;
		break;
	case ValueType::Int:
		val = value_change.valueX.intValue;
		break;
	case ValueType::Double:
		val = value_change.valueX.doubleValue * m_xValueScale;
		break;
	default:
		break;
	}
	return val;
}

QString GraphView::xValueString(quint64 value, const QString &datetime_format) const
{
	QString s;
	switch (settings.xAxisType) {
	case ValueType::TimeStamp:
		s = QDateTime(QDate::currentDate(), QTime()).addMSecs(value).toString(datetime_format);
		break;
	case ValueType::Int:
		s = QString::number(value);
		break;
	case ValueType::Double:
		s = QString::number(value / m_xValueScale);
		break;
	default:
		break;
	}
	return s;
}

void GraphView::computeRange(double &min, double &max, const Serie &serie)
{
	if (serie.dataPtr->size()) {
		if (serie.dataPtr->at(0).valueX.doubleValue < min) {
			min = serie.dataPtr->at(0).valueX.doubleValue;
		}
		if (serie.dataPtr->back().valueX.doubleValue > max) {
			max = serie.dataPtr->back().valueX.doubleValue;
		}
	}
}

void GraphView::computeRange(double &min, double &max)
{
	min = DBL_MAX;
	max = DBL_MIN;

	for (const Serie &serie : m_series) {
		computeRange(min, max, serie);
		for (const Serie &dependent_serie : serie.dependentSeries) {
			computeRange(min, max, dependent_serie);
		}
	}
	if (min == DBL_MAX && max == DBL_MIN) {
		min = max = 0.0;
	}
}

void GraphView::computeRange(int &min, int &max, const Serie &serie)
{
	if (serie.dataPtr->size()) {
		if (serie.dataPtr->at(0).valueX.intValue < min) {
			min = serie.dataPtr->at(0).valueX.intValue;
		}
		if (serie.dataPtr->back().valueX.intValue > max) {
			max = serie.dataPtr->back().valueX.intValue;
		}
	}
}

void GraphView::computeRange(int &min, int &max)
{
	min = INT_MAX;
	max = INT_MIN;

	for (const Serie &serie : m_series) {
		computeRange(min, max, serie);
		for (const Serie &dependent_serie : serie.dependentSeries) {
			computeRange(min, max, dependent_serie);
		}
	}
	if (min == INT_MAX && max == INT_MIN) {
		min = max = 0;
	}
}

void GraphView::computeRange(quint64 &min, quint64 &max, const Serie &serie)
{
	if (serie.dataPtr->size()) {
		if (serie.dataPtr->at(0).valueX.timeStamp < min) {
			min = serie.dataPtr->at(0).valueX.timeStamp;
		}
		if (serie.dataPtr->back().valueX.timeStamp > max) {
			max = serie.dataPtr->back().valueX.timeStamp;
		}
	}
}

void GraphView::computeRange(quint64 &min, quint64 &max)
{
	min = UINT64_MAX;
	max = 0;

	for (const Serie &serie : m_series) {
		computeRange(min, max, serie);
		for (const Serie &dependent_serie : serie.dependentSeries) {
			computeRange(min, max, dependent_serie);
		}
	}
	if (min == UINT64_MAX) {
		min = 0;
	}
}

SerieData::const_iterator GraphView::findMinYValue(const SerieData &data, quint64 x_value) const
{
	auto it = std::lower_bound(data.begin(), data.end(), x_value, [this](const ValueChange &data, quint64 value) {
	   return xValue(data) < value;
	});
	if (it != data.begin()) {
		--it;
	}
	return it;
}

RangeSelectorHandle::RangeSelectorHandle(QWidget *parent) : QPushButton(parent)
{
	setCursor(Qt::SizeHorCursor);
	setFixedSize(9, 20);
}

void RangeSelectorHandle::paintEvent(QPaintEvent *event)
{
	QPushButton::paintEvent(event);

	QPainter painter(this);
	painter.drawLine(3, 3, 3, height() - 6);
	painter.drawLine(width() - 4, 3, width() - 4, height() - 6);
}

bool GraphView::Selection::containsValue(quint64 value) const
{
	return ((start <= end && value >= start && value <= end) ||	(start > end && value >= end && value <= start));
}

} //namespace
}
