#include "visucontroller.h"
#include "ams005ycpapp.h"

#include <shv/coreqt/log.h>
#include <shv/visu/svgscene/simpletextitem.h>
#include <shv/chainpack/rpcvalue.h>

#include <QPainter>

namespace cp = shv::chainpack;
/*
static std::string transform2string(const QTransform &t)
{
	std::string ret;
	ret += std::to_string(t.m11()) + '\t';
	ret += std::to_string(t.m12()) + '\t';
	ret += std::to_string(t.m13()) + '\n';

	ret += std::to_string(t.m21()) + '\t';
	ret += std::to_string(t.m22()) + '\t';
	ret += std::to_string(t.m23()) + '\n';

	ret += std::to_string(t.m31()) + '\t';
	ret += std::to_string(t.m32()) + '\t';
	ret += std::to_string(t.m33()) + '\n';

	return ret;
}
*/
//===========================================================================
// VisuController
//===========================================================================
const QString VisuController::ATTR_CHILD_ID = QStringLiteral("chid");
const QString VisuController::ATTR_SHV_PATH = QStringLiteral("shvPath");
const QString VisuController::ATTR_SHV_TYPE = QStringLiteral("shvType");

VisuController::VisuController(QGraphicsItem *parent)
	: Super(parent)
{
}

void VisuController::updateValue()
{
	Ams005YcpApp *app = Ams005YcpApp::instance();
	QVariant val = app->opcValue(opcPath());
	onOpcValueChanged(opcPath(), val);
}

void VisuController::reload()
{
	updateValue();
	Ams005YcpApp *app = Ams005YcpApp::instance();
	app->reloadOpcValue(opcPath());
}

const std::string &VisuController::opcPath()
{
	return m_opcPath;
}

void VisuController::init()
{
	svgscene::XmlAttributes attrs = qvariant_cast<svgscene::XmlAttributes>(m_graphicsItem->data(svgscene::Types::DataKey::XmlAttributes));
	m_opcPath = attrs.value(ATTR_SHV_PATH).toStdString();
}


//===========================================================================
// RouteVisuController
//===========================================================================
RouteVisuController::RouteVisuController(QGraphicsItem *parent)
	: Super(parent)
{
}

void RouteVisuController::onOpcValueChanged(const std::string &path, const QVariant &val)
{
	if(opcPath() == path) {
		shvDebug() << __FUNCTION__ << opcPath() << "<<<" << path << "-->" << val.toString() << findChildGraphicsItem<QGraphicsSimpleTextItem*>(ATTR_CHILD_ID, QStringLiteral("value"));
		if(auto *value_item = findChildGraphicsItem<svgscene::SimpleTextItem*>(ATTR_CHILD_ID, QStringLiteral("value"))) {
			if(val.isValid()) {
				//value_item->setPen(QPen(Qt::green));
				value_item->setBrush(QBrush(Qt::magenta));
				QString txt = val.toString();
				value_item->setText(txt);
			}
			else {
				//value_item->setPen(QPen(Qt::green));
				value_item->setBrush(QBrush(Qt::darkGray));
				value_item->setText("---");
			}
		}
		if(auto *item = findChildGraphicsItem<QGraphicsEllipseItem*>()) {
			if(val.isValid()) {
				item->setBrush(QBrush(Qt::white));
			}
			else {
				item->setBrush(QBrush(Qt::darkGray));
			}
		}
		if(auto *item = findChildGraphicsItem<QGraphicsRectItem*>()) {
			if(val.isValid()) {
				item->setBrush(QBrush(Qt::white));
			}
			else {
				item->setBrush(QBrush(Qt::darkGray));
			}
		}
	}
}

void RouteVisuController::init()
{
	Super::init();
	if(auto *value_item = findChildGraphicsItem<svgscene::SimpleTextItem*>(ATTR_CHILD_ID, QStringLiteral("value"))) {
		svgscene::XmlAttributes attrs = qvariant_cast<svgscene::XmlAttributes>(value_item->data(svgscene::Types::DataKey::XmlAttributes));
		//m_suffix = attrs.value(QStringLiteral("shv_suffix"));
	}
}

//===========================================================================
// PushButtonVisuController
//===========================================================================
PushButtonVisuController::PushButtonVisuController(QGraphicsItem *parent)
	: Super(parent)
{
	shvLogFuncFrame();
}

void PushButtonVisuController::init()
{
	Super::init();
	if(auto *box = findChildGraphicsItem<QGraphicsRectItem*>(ATTR_CHILD_ID, QStringLiteral("box"))) {
		svgscene::XmlAttributes attrs = qvariant_cast<svgscene::XmlAttributes>(box->data(svgscene::Types::DataKey::XmlAttributes));
		//m_bitMask = attrs.value(QStringLiteral("shv_mask")).toUInt();
	}
	if(auto *bar = findChildGraphicsItem<QGraphicsPathItem*>(ATTR_CHILD_ID, QStringLiteral("bar"))) {
		//m_originalTrasformation = bar->transform();
		//shvDebug() << "original transformation: \n" << transform2string(m_originalTrasformation);
	}
}

void PushButtonVisuController::onOpcValueChanged(const std::string &path, const QVariant &val)
{
	if(opcPath() == path) {
		shvDebug() << __FUNCTION__ << opcPath() << "<<<" << path << "-->" << val.toString();
		unsigned status = val.toUInt();
		bool is_on = status & 2;
		shvDebug() << "OCL on:" << is_on;
		if(auto *box = findChildGraphicsItem<QGraphicsRectItem*>(ATTR_CHILD_ID, QStringLiteral("box"))) {
			if(val.isValid()) {
				box->setBrush(is_on? Qt::red: Qt::green);
			}
			else {
				box->setBrush(QColor(Qt::darkGray));
			}
		}
	}
}



