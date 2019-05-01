#pragma once

#include "graph.h"

#include <shv/coreqt/utils.h>

#include <QWidget>

namespace timeline {

class Graph;

class GraphWidget : public QWidget
{
	Q_OBJECT

	using Super = QWidget;
	//friend class GraphWidget;
public:
	GraphWidget(QWidget *parent = nullptr);

	void setModel(GraphModel *m);
	Graph *graph();

	void makeLayout(const QRect &rect);

	// QWidget interface
protected:
	void paintEvent(QPaintEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
protected:
	Graph m_graph;
};

} // namespace timeline
