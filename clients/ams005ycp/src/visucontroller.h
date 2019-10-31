#ifndef VISUCONTROLLER_H
#define VISUCONTROLLER_H

#include <shv/visu/svgscene/types.h>
#include <shv/visu/svgscene/saxhandler.h>
#include <shv/visu/svgscene/visucontroller.h>

#include <QGraphicsItem>

namespace shv { namespace chainpack { class RpcValue; }}
namespace svgscene = shv::visu::svgscene;

class VisuController : public svgscene::VisuController
{
	Q_OBJECT

	using Super = svgscene::VisuController;
public:
	VisuController(QGraphicsItem *graphics_item, QObject *parent = nullptr);

	QString opcPath() const {return shvPath();}
	virtual void onOpcValueChanged(const QString &opc_path, const QVariant &val) = 0;
	virtual void updateValue();
	virtual void reload();
};

class RouteVisuController : public VisuController
{
	Q_OBJECT
	using Super = VisuController;
public:
	RouteVisuController(QGraphicsItem *graphics_item, QObject *parent = nullptr);

	//void init() override;

	void onOpcValueChanged(const QString &opcPath, const QVariant &val) override;
protected:
};


class PushButtonVisuController : public VisuController
{
	Q_OBJECT
	using Super = VisuController;
public:
	PushButtonVisuController(QGraphicsItem *graphics_item, QObject *parent = nullptr);

	void onOpcValueChanged(const QString &opcPath, const QVariant &val) override;
	void onPressedChanged(bool is_down);
private:
	QColor m_savedColor;
	QAbstractGraphicsShapeItem *m_pushAreaItem;
};

#endif // VISUCONTROLLER_H
