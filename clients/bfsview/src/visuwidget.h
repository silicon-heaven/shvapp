#ifndef VISUWIDGET_H
#define VISUWIDGET_H

#include "bfsviewapp.h"

#include <QWidget>
#include <QDomDocument>

#include <functional>

class QSvgRenderer;
class SwitchVisuController;

class VisuWidget : public QWidget
{
	Q_OBJECT

	using Super = QWidget;
	friend class SwitchVisuController;
public:
	VisuWidget(QWidget *parent);
	~VisuWidget() override;

	bool load(const QByteArray &svg);
	void refreshVisualization();
protected:
	void paintEvent(QPaintEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;
private:
	QDomElement elementById(const QString &id);
	QDomElement elementByShvName(const QDomElement &parent_el, const QString &shv_name);
	void setElementText(const QString &elem_id, const QString &text);
	void setElementColor(const QString &id, const QString &c);
	void setElementFillColor(const QString &id, const QString &c);
	void setElementFillColor(QDomElement el, const QString &c);
	void setElementVisible(QDomElement el, bool on);
	void setElementVisible(const QString &elem_id, bool on);
	void setElementStyleAttribute(const QString &id, const QString &key, const QString &val);
	void setElementStyleAttribute(QDomElement el, const QString &key, const QString &val);

	QRect svgRectToWidgetRect(const QRectF &svg_rect);
private:
	QSvgRenderer *m_renderer;
	QDomDocument m_xDoc;
	QRectF m_ompagRect;
	QRectF m_convRect;

	SwitchVisuController *m_ompagVisuController;
	SwitchVisuController *m_convVisuController;
};

class SwitchVisuController : public QObject
{
	Q_OBJECT
public:
	SwitchVisuController(const QString &xml_id, VisuWidget *vw, std::function<BfsViewApp::SwitchStatus (BfsViewApp*)> sfn);

	void updateXml();

	void onRequiredSwitchStatusChanged(int st);
private:
	QString statusToColor(BfsViewApp::SwitchStatus status);
private:
	QString m_xmlId;
	VisuWidget *m_visuWidget;
	std::function<BfsViewApp::SwitchStatus (BfsViewApp*)> m_statusFn;
	BfsViewApp::SwitchStatus m_requiredSwitchStatus = BfsViewApp::SwitchStatus::Unknown;
	bool m_toggleBit = false;
	QTimer *m_blinkTimer = nullptr;
};

#endif // VISUWIDGET_H
