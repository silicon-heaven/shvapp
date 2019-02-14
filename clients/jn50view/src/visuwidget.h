#ifndef VISUWIDGET_H
#define VISUWIDGET_H

#include "jn50viewapp.h"
#include "svgscene/graphicsview.h"

#include <QWidget>
#include <QGraphicsItem>

#include <functional>

class QTimer;
class QGraphicsScene;
class SwitchVisuController;

class VisuWidget : public svgscene::GraphicsView
{
	Q_OBJECT

	using Super = svgscene::GraphicsView;
	friend class SwitchVisuController;
public:
	VisuWidget(QWidget *parent);
	~VisuWidget() override;

	bool load(const QString &file_name);
	void zoomToFitDeferred();

	void paintEvent(QPaintEvent *event) override;
	//void onShvDeviceValueChanged(const std::string &path, const shv::chainpack::RpcValue &val);
private:
	QGraphicsScene *m_scene;
	QTimer *m_scaleToFitTimer = nullptr;
};

#endif // VISUWIDGET_H
