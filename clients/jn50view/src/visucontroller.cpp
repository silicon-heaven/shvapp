#include "visucontroller.h"
#include "jn50viewapp.h"

#include <shv/coreqt/log.h>

#include <shv/chainpack/rpcvalue.h>

#include <QPainter>

namespace cp = shv::chainpack;

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

const std::string &VisuController::shvPath()
{
	if(!m_shvPathLoaded) {
		m_shvPathLoaded = true;
		svgscene::XmlAttributes attrs = qvariant_cast<svgscene::XmlAttributes>(data(svgscene::XmlAttributesKey));
		m_shvPath = attrs.value(ATTR_SHV_PATH).toStdString();
	}
	return m_shvPath;
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
		bool ocl_on = status & Jn50ViewApp::ConvStatus::OCL_VOLTAGE;
		shvDebug() << "OCL on:" << ocl_on;
		if(auto *box = findChild<QGraphicsRectItem*>(ATTR_CHILD_ID, QStringLiteral("box"))) {
			box->setBrush(ocl_on? Qt::green: Qt::darkGray);
		}
		if(auto *bar = findChild<QGraphicsPathItem*>(ATTR_CHILD_ID, QStringLiteral("bar"))) {
			if(!m_originalTrasformationLoaded) {
				m_originalTrasformationLoaded = true;
				m_originalTrasformation = bar->transform();
				m_originalRotation = bar->rotation();
				//transformOriginPoint()
				shvDebug() << "original transformation: \n" << transform2string(m_originalTrasformation);
			}
			if(ocl_on) {
				QPointF bl = bar->boundingRect().bottomLeft();
#if 1
				QTransform tfm = m_originalTrasformation;
				tfm.translate(bl.x(), bl.y());
				tfm.rotate(15);
				tfm.translate(-bl.x(), -bl.y());
				bar->setTransform(tfm);
				shvDebug() << "OCL on transformation: \n" << transform2string(bar->transform());
				//bar->setRotation(m_originalRotation + 30);
#else
				bar->setTransformOriginPoint(bl);
				bar->setRotation(30);
#endif
			}
			else {
				bar->setTransform(m_originalTrasformation);
				shvDebug() << "OCL off transformation: \n" << transform2string(bar->transform());
				//bar->setRotation(m_originalRotation);
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
#if 0
void SwitchVisuController::onRequiredSwitchStatusChanged(Jn50ViewApp::SwitchStatus st)
{
	Jn50ViewApp *app = Jn50ViewApp::instance();
	Jn50ViewApp::SwitchStatus status = m_statusFn(app);
	m_requiredSwitchStatus = st;
	if(m_requiredSwitchStatus == status) {
		if(m_blinkTimer)
			m_blinkTimer->stop();
	}
	else {
		//m_requiredStatus = st;
		if(!m_blinkTimer) {
			m_blinkTimer = new QTimer(this);
			connect(m_blinkTimer, &QTimer::timeout, [this]() {
				m_toggleBit = !m_toggleBit;
			});
		}
		m_blinkTimer->start(500);
	}
}

QString SwitchVisuController::statusToColor(Jn50ViewApp::SwitchStatus status)
{
	if(m_requiredSwitchStatus != Jn50ViewApp::SwitchStatus::Unknown) {
		Jn50ViewApp *app = Jn50ViewApp::instance();
		Jn50ViewApp::SwitchStatus status = m_statusFn(app);
		if(status != m_requiredSwitchStatus) {
			if(m_toggleBit)
				return QStringLiteral("orangered");
		}
	}
	return ::switchStatusToColor(status);
}
#endif

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
		if(auto *value_item = findChild<QGraphicsSimpleTextItem*>(ATTR_CHILD_ID, QStringLiteral("value"))) {
			value_item->setText(QString::fromStdString(val.toCpon()));
		}
	}
}
