#ifndef VISUWIDGET_H
#define VISUWIDGET_H

#include <QWidget>
#include <QDomDocument>

class QSvgRenderer;

class VisuWidget : public QWidget
{
	Q_OBJECT

	using Super = QWidget;
public:
	VisuWidget(QWidget *parent);

	bool load(const QByteArray &svg);
	void refreshVisualization();
protected:
	void paintEvent(QPaintEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
private:
	QDomElement elementById(const QString &id);
	void setElementText(const QString &elem_id, const QString &text);
	void setElementColor(const QString &id, const QString &c);
	void setElementFillColor(const QString &id, const QString &c);
	void setElementVisible(const QString &elem_id, bool on);
	void setElementStyleAttribute(const QString &elem_id, const QString &key, const QString &val);

	QRect svgRectToWidgetRect(const QRectF &svg_rect);
	int ompagStatus();
	int mswStatus();
private:
	QSvgRenderer *m_renderer;
	QDomDocument m_xDoc;
	QRectF m_ompagRect;
	QRectF m_mswRect;
};

#endif // VISUWIDGET_H
