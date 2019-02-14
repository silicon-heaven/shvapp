#include "svghandler.h"
#include "visucontroller.h"
#include "jn50viewapp.h"

#include <shv/coreqt/log.h>

SvgHandler::SvgHandler(QGraphicsScene *scene)
	: Super(scene)
{

}

QGraphicsItem *SvgHandler::createGroupItem(const svgscene::SaxHandler::SvgElement &el)
{
	shvLogFuncFrame() << el.name << el.xmlAttributes.value("shvType");
	const QString shv_type = el.xmlAttributes.value(QStringLiteral("shvType"));
	if(shv_type == QLatin1String("switch")) {
		auto *item = new SwitchVisuController();
		QObject::connect(Jn50ViewApp::instance(), &Jn50ViewApp::shvDeviceValueChanged, item, &SwitchVisuController::onShvDeviceValueChanged);
		return item;
	}
	else if(shv_type == QLatin1String("multimeter")) {
		auto *item = new MultimeterVisuController();
		QObject::connect(Jn50ViewApp::instance(), &Jn50ViewApp::shvDeviceValueChanged, item, &MultimeterVisuController::onShvDeviceValueChanged);
		return item;
	}
	return Super::createGroupItem(el);
}
