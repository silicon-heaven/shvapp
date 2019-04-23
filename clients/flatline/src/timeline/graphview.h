#pragma once

#include "graph.h"

#include <QPointer>
#include <QScrollArea>
#include <QVector>

namespace timeline {

class GraphModel;

class GraphView : public QScrollArea
{
	Q_OBJECT

	using Super = QScrollArea;
public:
	using timemsec_t = Graph::timemsec_t;
	struct Options
	{
		int unitToPx = 0;
		QColor colorBackground = Qt::black;
		QColor colorGrid = Qt::green;
	};
	struct Channel
	{
		int metaTypeId = 0;

		double yRangeMin = 0;
		double yRangeMax = 0;
		double yRange() const { return yRangeMax - yRangeMin; }

		double height = 1; // one unit
		QColor color;
	};
	struct State
	{
		timemsec_t xRangeMin = 0;
		timemsec_t xRangeMax = 0;
		double xRange() const { return xRangeMax - xRangeMin; }
	};

public:
	GraphView(QWidget *parent = nullptr);

	void setModel(GraphModel *model);
	void createChannelsFromModel();
protected:
	virtual void drawGraph(int channel);
	virtual void drawSamples(int channel, const QRect &dest_rect);
	virtual void drawXAxis(int channel);
	virtual void drawYAxis(int channel);

	// QWidget interface
protected:
	void paintEvent(QPaintEvent *event) override;
protected:
	QVector<Channel> m_channels;
	Options m_options;
	State m_state;

	QPointer<GraphModel> m_model;
};

} // namespace timeline
