#include "svghandler.h"
#include "visucontroller.h"
#include "ams005ycpapp.h"

#include <shv/coreqt/log.h>

#include <QSet>
#include <QGraphicsScene>

//#define logSvgM() nCMessage("svg")
namespace svgscene = shv::visu::svgscene;

SvgHandler::SvgHandler(QGraphicsScene *scene)
	: Super(scene)
{

}

void SvgHandler::createVisuController(QGraphicsItem *it, const svgscene::SaxHandler::SvgElement &el)
{
	const QString shv_type = el.xmlAttributes.value(shv::visu::svgscene::Types::ATTR_SHV_TYPE);
	if(shv_type == QLatin1String("Route")) {
		shvDebug() << it << el.name;
	}
	else if(shv_type == QLatin1String("PushButton")) {
		shvDebug() << it << el.name;
		new PushButtonVisuController(it, it->scene());
		//vc->init();
	}
}

/*
QGraphicsItem *SvgHandler::createGroupItem(const svgscene::SaxHandler::SvgElement &el)
{
	shvLogFuncFrame() << el.name << el.xmlAttributes.value("shvType");
	QGraphicsRectItem *item = nullptr;
	const QString shv_type = el.xmlAttributes.value(QStringLiteral("shvType"));
	const QString shv_path = el.xmlAttributes.value(QStringLiteral("shvPath"));
	if(shv_type == QLatin1String("PushButton")) {
		item = new PushButtonVisuController();
	}
	else if(shv_type == QLatin1String("Route")) {
		item = new RouteVisuController();
	}
	if(item) {
		shvDebug() << "created:" << item->metaObject()->className();
		QObject::connect(Ams005YcpApp::instance(), &Ams005YcpApp::opcValueChanged, item, &VisuController::onOpcValueChanged);
		return item;
	}
	return Super::createGroupItem(el);
}

void SvgHandler::setXmlAttributes(QGraphicsItem *git, const svgscene::SaxHandler::SvgElement &el)
{
	svgscene::XmlAttributes attrs = qvariant_cast<svgscene::XmlAttributes>(git->data(svgscene::Types::DataKey::XmlAttributes));
	static QSet<QString> known_attrs {"shvPath", "shvType", "chid"};
	QMapIterator<QString, QString> it(el.xmlAttributes);
	while (it.hasNext()) {
		it.next();
		if(known_attrs.contains(it.key()))
			attrs[it.key()] = it.value();
		if(it.key().startsWith(QStringLiteral("shv_")))
			attrs[it.key()] = it.value();
	}
	git->setData(svgscene::Types::DataKey::XmlAttributes, QVariant::fromValue(attrs));
}
*/


