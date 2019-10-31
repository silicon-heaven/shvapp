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
VisuController::VisuController(QGraphicsItem *graphics_item, QObject *parent)
	: Super(graphics_item, parent)
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

//===========================================================================
// RouteVisuController
//===========================================================================
RouteVisuController::RouteVisuController(QGraphicsItem *graphics_item, QObject *parent)
	: Super(graphics_item, parent)
{
}

void RouteVisuController::onOpcValueChanged(const QString &path, const QVariant &val)
{
	if(opcPath() == path) {
		shvDebug() << __FUNCTION__ << opcPath() << "<<<" << path << "-->" << val.toString()
				   << findChildGraphicsItem<QGraphicsSimpleTextItem*>(svgscene::Types::ATTR_CHILD_ID, QStringLiteral("value"));
		if(auto *value_item = findChildGraphicsItem<svgscene::SimpleTextItem*>(svgscene::Types::ATTR_CHILD_ID, QStringLiteral("value"))) {
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
/*
void RouteVisuController::init()
{
	Super::init();
	if(auto *value_item = findChildGraphicsItem<svgscene::SimpleTextItem*>(svgscene::Types::ATTR_CHILD_ID, QStringLiteral("value"))) {
		svgscene::XmlAttributes attrs = qvariant_cast<svgscene::XmlAttributes>(value_item->data(svgscene::Types::DataKey::XmlAttributes));
		//m_suffix = attrs.value(QStringLiteral("shv_suffix"));
	}
}
*/
//===========================================================================
// PushButtonSceneEventFilter
//===========================================================================
class PushButtonSceneEventFilter : public QGraphicsRectItem
{
	using Super = QGraphicsRectItem;
public:
	PushButtonSceneEventFilter(PushButtonVisuController *visu_controller, QGraphicsItem *parent = nullptr)
		: Super(parent)
		, m_visuController(visu_controller)
	{
		setBrush(Qt::red);
		static constexpr int W = 2;
		setRect(-W/2, W/2, W, W);
		//setPos(parent->pos());
	}
protected:
	bool sceneEventFilter(QGraphicsItem *watched, QEvent *event) override;
private:
	PushButtonVisuController *m_visuController;
};

bool PushButtonSceneEventFilter::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
	if(event->type() == QEvent::GraphicsSceneMousePress) {
		m_visuController->onPressedChanged(true);
		return true;
	}
	if(event->type() == QEvent::GraphicsSceneMouseRelease) {
		m_visuController->onPressedChanged(false);
		return true;
	}
	return Super::sceneEventFilter(watched, event);
}

//===========================================================================
// PushButtonVisuController
//===========================================================================
PushButtonVisuController::PushButtonVisuController(QGraphicsItem *graphics_item, QObject *parent)
	: Super(graphics_item, parent)
{
	shvLogFuncFrame() << graphics_item;
	auto *event_filter = new PushButtonSceneEventFilter(this, graphics_item);
	//auto *it = findChildGraphicsItem<QGraphicsItem*>(svgscene::Types::ATTR_CHILD_ID, QStringLiteral("pushArea"));
	//if(it)
	//	shvDebug() << it << typeid (*it).name();
	m_pushAreaItem = findChildGraphicsItem<QAbstractGraphicsShapeItem*>(svgscene::Types::ATTR_CHILD_ID, QStringLiteral("pushArea"));
	if(m_pushAreaItem) {
		//svgscene::XmlAttributes attrs = qvariant_cast<svgscene::XmlAttributes>(box->data(svgscene::Types::DataKey::XmlAttributes));
		//m_bitMask = attrs.value(QStringLiteral("shv_mask")).toUInt();
		m_pushAreaItem->installSceneEventFilter(event_filter);
		shvDebug() << "push_area:" << m_pushAreaItem;
	}
	if(auto *label = findChildGraphicsItem<svgscene::SimpleTextItem*>(svgscene::Types::ATTR_CHILD_ID, QStringLiteral("label"))) {
		//m_originalTrasformation = bar->transform();
		shvDebug() << "push button label:" << label->text();
	}
}

void PushButtonVisuController::onOpcValueChanged(const QString &path, const QVariant &val)
{
	if(opcPath() == path) {
		shvDebug() << __FUNCTION__ << opcPath() << "<<<" << path << "-->" << val.toString();
		unsigned status = val.toUInt();
		bool is_on = status & 2;
		shvDebug() << "OCL on:" << is_on;
		if(auto *box = findChildGraphicsItem<QGraphicsRectItem*>(svgscene::Types::ATTR_CHILD_ID, QStringLiteral("box"))) {
			if(val.isValid()) {
				box->setBrush(is_on? Qt::red: Qt::green);
			}
			else {
				box->setBrush(QColor(Qt::darkGray));
			}
		}
	}
}

void PushButtonVisuController::onPressedChanged(bool is_down)
{
	shvInfo() << id() << "pressed:" << is_down;
	if(is_down) {
		m_savedColor = m_pushAreaItem->brush().color();
		m_pushAreaItem->setBrush(m_savedColor.darker());
	}
	else {
		m_pushAreaItem->setBrush(m_savedColor);
	}
}



