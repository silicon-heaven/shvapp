#pragma once

#include <QWidget>

namespace timeline {

class GraphView;

class GraphWidget : public QWidget
{
	Q_OBJECT

	using Super = QWidget;
public:
	explicit GraphWidget(QWidget *parent = nullptr);

	// QWidget interface
protected:
	void paintEvent(QPaintEvent *event) override;
private:
	//GraphView *m_graphView;
};

} // namespace timeline

