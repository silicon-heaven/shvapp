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
	shv::chainpack::RpcValue val = app->shvDeviceValue(shvPath());
	onShvDeviceValueChanged(shvPath(), val);
}

void VisuController::reload()
{
	updateValue();
	Ams005YcpApp *app = Ams005YcpApp::instance();
	app->reloadShvDeviceValue(shvPath());
}

const std::string &VisuController::shvPath()
{
	return m_shvPath;
}

void VisuController::init()
{
	svgscene::XmlAttributes attrs = qvariant_cast<svgscene::XmlAttributes>(data(svgscene::XmlAttributesKey));
	m_shvPath = attrs.value(ATTR_SHV_PATH).toStdString();
}

//===========================================================================
// SwitchVisuController
//===========================================================================
StatusVisuController::StatusVisuController(QGraphicsItem *parent)
	: Super(parent)
{

}

void StatusVisuController::init()
{
	Super::init();
	if(auto *box = findChild<QGraphicsRectItem*>(ATTR_CHILD_ID, QStringLiteral("box"))) {
		svgscene::XmlAttributes attrs = qvariant_cast<svgscene::XmlAttributes>(box->data(svgscene::XmlAttributesKey));
		m_bitMask = attrs.value(QStringLiteral("shv_mask")).toUInt();
		QString color_on = attrs.value(QStringLiteral("shv_colorOn"));
		m_colorOn = color_on.isEmpty()? QColor("limegreen"): QColor(color_on);
		QString color_off = attrs.value(QStringLiteral("shv_colorOff"));
		m_colorOff = color_off.isEmpty()? QColor(Qt::red): QColor(color_off);
		//shvInfo() << this->metaObject()->className() << "on:" << color_on << "off:" << color_off;
	}
}

//===========================================================================
// StatusBoxVisuController
//===========================================================================
StatusBitVisuController::StatusBitVisuController(QGraphicsItem *parent)
	: Super(parent)
{

}

void StatusBitVisuController::onShvDeviceValueChanged(const std::string &path, const shv::chainpack::RpcValue &val)
{
	if(shvPath() == path) {
		shvDebug() << __FUNCTION__ << shvPath() << "<<<" << path << "-->" << val.toCpon();
		if(auto *box = findChild<QGraphicsRectItem*>(ATTR_CHILD_ID, QStringLiteral("box"))) {
			if(val.isValid()) {
				unsigned status = val.toUInt();
				bool is_on = status & m_bitMask;
				shvDebug() << "on:" << is_on;
				box->setBrush(is_on? m_colorOn: m_colorOff);
			}
			else {
				box->setBrush(QColor(Qt::darkGray));
			}
		}
	}
}
//===========================================================================
// SwitchVisuController
//===========================================================================
SwitchVisuController::SwitchVisuController(QGraphicsItem *parent)
	: Super(parent)
{
	shvLogFuncFrame();
}

void SwitchVisuController::onShvDeviceValueChanged(const std::string &path, const shv::chainpack::RpcValue &val)
{
	if(shvPath() == path) {
		shvDebug() << __FUNCTION__ << shvPath() << "<<<" << path << "-->" << val.toCpon();
		unsigned status = val.toUInt();
		bool is_on = status & m_bitMask;
		shvDebug() << "OCL on:" << is_on;
		if(auto *box = findChild<QGraphicsRectItem*>(ATTR_CHILD_ID, QStringLiteral("box"))) {
			if(val.isValid()) {
				box->setBrush(is_on? m_colorOn: m_colorOff);
			}
			else {
				box->setBrush(QColor(Qt::darkGray));
			}
		}
		if(auto *bar = findChild<QGraphicsPathItem*>(ATTR_CHILD_ID, QStringLiteral("bar"))) {
			bar->setVisible(val.isValid());
			if(val.isValid()) {
				if(is_on) {
					bar->setTransform(m_originalTrasformation);
					//shvDebug() << "OCL off transformation: \n" << transform2string(bar->transform());
					QPen pen = bar->pen();
					pen.setColor(Qt::white);
					bar->setPen(pen);
				}
				else {
					QPointF bl = bar->boundingRect().bottomLeft();
					QTransform tfm = m_originalTrasformation;
					tfm.translate(bl.x(), bl.y());
					tfm.rotate(15);
					tfm.translate(-bl.x(), -bl.y());
					bar->setTransform(tfm);
					//shvDebug() << "OCL on transformation: \n" << transform2string(bar->transform());
					QPen pen = bar->pen();
					pen.setColor(Qt::white);
					bar->setPen(pen);
				}
			}
		}
	}
}

void SwitchVisuController::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Super::paint(painter, option, widget);
	/*
	if(auto *bar = findChild<QGraphicsPathItem*>(ATTR_CHILD_ID, QStringLiteral("bar"))) {
		painter->setPen(Qt::red);
		painter->drawRect(bar->boundingRect());
		painter->drawLine(QPoint(0, 0), bar->boundingRect().bottomLeft());
	}
	*/
}

void SwitchVisuController::init()
{
	Super::init();
	if(auto *box = findChild<QGraphicsRectItem*>(ATTR_CHILD_ID, QStringLiteral("box"))) {
		svgscene::XmlAttributes attrs = qvariant_cast<svgscene::XmlAttributes>(box->data(svgscene::XmlAttributesKey));
		m_bitMask = attrs.value(QStringLiteral("shv_mask")).toUInt();
	}
	if(auto *bar = findChild<QGraphicsPathItem*>(ATTR_CHILD_ID, QStringLiteral("bar"))) {
		m_originalTrasformation = bar->transform();
		//shvDebug() << "original transformation: \n" << transform2string(m_originalTrasformation);
	}
}

//===========================================================================
// MultimeterVisuController
//===========================================================================
MultimeterVisuController::MultimeterVisuController(QGraphicsItem *parent)
	: Super(parent)
{
}

void MultimeterVisuController::onShvDeviceValueChanged(const std::string &path, const shv::chainpack::RpcValue &val)
{
	if(shvPath() == path) {
		shvDebug() << __FUNCTION__ << shvPath() << "<<<" << path << "-->" << val.toCpon() << findChild<QGraphicsSimpleTextItem*>(ATTR_CHILD_ID, QStringLiteral("value"));
		if(auto *value_item = findChild<svgscene::SimpleTextItem*>(ATTR_CHILD_ID, QStringLiteral("value"))) {
			if(val.isValid()) {
				//value_item->setPen(QPen(Qt::green));
				value_item->setBrush(QBrush(Qt::magenta));
				QString txt = QString::fromStdString(val.toCpon());
				if(!m_suffix.isEmpty())
					txt += ' ' + m_suffix;
				value_item->setText(txt);
			}
			else {
				//value_item->setPen(QPen(Qt::green));
				value_item->setBrush(QBrush(Qt::darkGray));
				value_item->setText("---");
			}
		}
		if(auto *item = findChild<QGraphicsEllipseItem*>()) {
			if(val.isValid()) {
				item->setBrush(QBrush(Qt::white));
			}
			else {
				item->setBrush(QBrush(Qt::darkGray));
			}
		}
		if(auto *item = findChild<QGraphicsRectItem*>()) {
			if(val.isValid()) {
				item->setBrush(QBrush(Qt::white));
			}
			else {
				item->setBrush(QBrush(Qt::darkGray));
			}
		}
	}
}

void MultimeterVisuController::init()
{
	Super::init();
	if(auto *value_item = findChild<svgscene::SimpleTextItem*>(ATTR_CHILD_ID, QStringLiteral("value"))) {
		svgscene::XmlAttributes attrs = qvariant_cast<svgscene::XmlAttributes>(value_item->data(svgscene::XmlAttributesKey));
		m_suffix = attrs.value(QStringLiteral("shv_suffix"));
	}
}


