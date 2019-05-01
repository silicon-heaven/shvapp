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
	const Graph *graph() const;

	void makeLayout(const QRect &rect);

	// QWidget interface
protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
protected:
	bool isMouseAboveMiniMapHandle(const QPoint &mouse_pos, int handle_pos) const;
	bool isMouseAboveLeftMiniMapHandle(const QPoint &pos) const;
	bool isMouseAboveRightMiniMapHandle(const QPoint &pos) const;
	bool isMouseAboveMiniMapSlider(const QPoint &pos) const;
protected:
	Graph m_graph;

	enum class MiniMapOperation { None, LeftResize, RightResize, Scroll };
	MiniMapOperation m_miniMapOperation = MiniMapOperation::None;
	int m_miniMapScrollPos = 0;
};

} // namespace timeline
