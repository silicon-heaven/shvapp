#ifndef VISUWIDGET_H
#define VISUWIDGET_H

#include "ams005ycpapp.h"

#include <shv/visu/svgscene/graphicsview.h>

#include <QWidget>
#include <QGraphicsItem>

#include <functional>

class QTimer;
class QGraphicsScene;
class VisuController;

class VisuWidget : public shv::visu::svgscene::GraphicsView
{
	Q_OBJECT

	using Super = shv::visu::svgscene::GraphicsView;
	friend class SwitchVisuController;
public:
	VisuWidget(QWidget *parent);
	~VisuWidget() override;

	bool load(const QString &file_name);
	void zoomToFitDeferred();

	void paintEvent(QPaintEvent *event) override;
	//void onShvDeviceValueChanged(const std::string &path, const shv::chainpack::RpcValue &val);
private:
	QList<VisuController*> findVisuControllers();
private:
	QGraphicsScene *m_scene;
	//QList<VisuController*> m_visuControllers;
	QTimer *m_scaleToFitTimer = nullptr;
};

#endif // VISUWIDGET_H
