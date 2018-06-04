#ifndef VISUWIDGET_H
#define VISUWIDGET_H

#include <QWidget>

class QSvgRenderer;

class VisuWidget : public QWidget
{
	Q_OBJECT

	using Super = QWidget;
public:
	VisuWidget(QWidget *parent);

	bool load(const QByteArray &svg);
protected:
	void paintEvent(QPaintEvent *event) override;
private:
	QSvgRenderer *m_renderer;
};

#endif // VISUWIDGET_H
