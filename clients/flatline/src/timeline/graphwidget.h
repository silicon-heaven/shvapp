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

	void setGraph(Graph *g);
	Graph *graph();
	const Graph *graph() const;

	void makeLayout(const QRect &rect);

	// QWidget interface
protected:
	void paintEvent(QPaintEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
protected:
	bool isMouseAboveMiniMapHandle(const QPoint &mouse_pos, bool left) const;
	bool isMouseAboveLeftMiniMapHandle(const QPoint &pos) const;
	bool isMouseAboveRightMiniMapHandle(const QPoint &pos) const;
	bool isMouseAboveMiniMapSlider(const QPoint &pos) const;
protected:
	Graph *m_graph = nullptr;

	enum class MiniMapOperation { None, LeftResize, RightResize, Scroll };
	MiniMapOperation m_miniMapOperation = MiniMapOperation::None;
	int m_miniMapScrollPos = 0;
};

} // namespace timeline
