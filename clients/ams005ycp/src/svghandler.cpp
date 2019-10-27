#include "svghandler.h"
#include "visucontroller.h"
#include "ams005ycpapp.h"

#include <shv/coreqt/log.h>

#include <QSet>

SvgHandler::SvgHandler(QGraphicsScene *scene)
	: Super(scene)
{

}

QGraphicsItem *SvgHandler::createGroupItem(const svgscene::SaxHandler::SvgElement &el)
{
	shvLogFuncFrame() << el.name << el.xmlAttributes.value("shvType");
	VisuController *item = nullptr;
	const QString shv_type = el.xmlAttributes.value(QStringLiteral("shvType"));
	const QString shv_path = el.xmlAttributes.value(QStringLiteral("shvPath"));
	if(shv_type == QLatin1String("switch")) {
		item = new SwitchVisuController();
		shvDebug() << "creating:" << item->metaObject()->className();
	}
	else if(shv_type == QLatin1String("statusbit")) {
		item = new StatusBitVisuController();
		shvDebug() << "creating:" << item->metaObject()->className();
	}
	else if(shv_type == QLatin1String("multimeter")) {
		item = new MultimeterVisuController();
		shvDebug() << "creating:" << item->metaObject()->className();
	}
	if(item) {
		QObject::connect(Ams005YcpApp::instance(), &Ams005YcpApp::opcValueChanged, item, &VisuController::onOpcValueChanged);
		return item;
	}
	return Super::createGroupItem(el);
}

void SvgHandler::setXmlAttributes(QGraphicsItem *git, const svgscene::SaxHandler::SvgElement &el)
{
	svgscene::XmlAttributes attrs = qvariant_cast<svgscene::XmlAttributes>(git->data(svgscene::XmlAttributesKey));
	static QSet<QString> known_attrs {"shvPath", "shvType", "chid"};
	QMapIterator<QString, QString> it(el.xmlAttributes);
	while (it.hasNext()) {
		it.next();
		if(known_attrs.contains(it.key()))
			attrs[it.key()] = it.value();
		if(it.key().startsWith(QStringLiteral("shv_")))
			attrs[it.key()] = it.value();
	}
	git->setData(svgscene::XmlAttributesKey, QVariant::fromValue(attrs));
}
