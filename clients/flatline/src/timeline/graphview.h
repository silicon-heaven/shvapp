#pragma once

#include <QScrollArea>

namespace timeline {

class GraphView : public QScrollArea
{
	Q_OBJECT

	using Super = QScrollArea;
public:
	GraphView(QWidget *parent = nullptr);

	void makeLayout();
protected:
	void resizeEvent(QResizeEvent *event) override;
};

} // namespace timeline

