#include "visuwidget.h"
#include "bfsviewapp.h"
#include "settingsdialog.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/deviceconnection.h>

#include <QCoreApplication>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QSvgRenderer>
#include <QTimer>

static QString r2s(const QRectF &r)
{
	return QString("(x: %1 y %2 w %3 h %4)").arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
}

VisuWidget::VisuWidget(QWidget *parent)
	: Super(parent)
	, m_renderer(new QSvgRenderer(this))
	, m_xDoc("SVG")
	, m_ompagVisuController(new SwitchVisuController(QStringLiteral("ctl_ompag"), this, &BfsViewApp::ompagSwitchStatus))
	, m_convVisuController(new SwitchVisuController(QStringLiteral("ctl_conv"), this, &BfsViewApp::convSwitchStatus))
{
	setMouseTracking(true);
	BfsViewApp *app = BfsViewApp::instance();
	connect(app, &BfsViewApp::bfsStatusChanged, m_ompagVisuController, &SwitchVisuController::updateXml);
	connect(app, &BfsViewApp::ompagRequiredSwitchStatusChanged, m_ompagVisuController, &SwitchVisuController::onRequiredSwitchStatusChanged);
	connect(app, &BfsViewApp::bfsStatusChanged, m_convVisuController, &SwitchVisuController::updateXml);
	connect(app, &BfsViewApp::convRequiredSwitchStatusChanged, m_convVisuController, &SwitchVisuController::onRequiredSwitchStatusChanged);
	//setContextMenuPolicy(Qt::CustomContextMenu);
	{
		QFile file(":/images/bfs1.svg");
		if (!file.open(QIODevice::ReadOnly))
			return;
		if (!m_xDoc.setContent(&file)) {
			shvError() << "Error parsing SVG file";
			QCoreApplication::instance()->quit();
		}
	}
}

VisuWidget::~VisuWidget()
{
	delete m_ompagVisuController;
	delete m_convVisuController;
}

bool VisuWidget::load(const QByteArray &svg)
{
	bool ret = m_renderer->load(svg);
	if(ret) {
		QDomElement el = elementById("sw_ompag_rect");
		if(!el.isNull()) {
			double x = el.attribute("x").toDouble();
			double y = el.attribute("y").toDouble();
			double h = el.attribute("height").toDouble();
			double w = el.attribute("width").toDouble();
			m_ompagRect = QRectF(x, y, w, h);
			shvDebug() << "x:" << el.attribute("x") << "ompag:" << r2s(m_ompagRect);
		}
		el = elementById("shv_rect_conv");
		if(!el.isNull()) {
			double x = el.attribute("x").toDouble();
			double y = el.attribute("y").toDouble();
			double h = el.attribute("height").toDouble();
			double w = el.attribute("width").toDouble();
			m_convRect = QRectF(x, y, w, h);
		}
		update();
	}
	return ret;
}

void VisuWidget::paintEvent(QPaintEvent *event)
{
	Super::paintEvent(event);
	QPainter p(this);
	QSize svg_sz = m_renderer->defaultSize();
	QRect widget_r = geometry();
	widget_r.moveTopLeft({0, 0});
	p.fillRect(widget_r, QColor("#2b4174"));
	widget_r.setSize(svg_sz.scaled(widget_r.size(), Qt::KeepAspectRatio));
	m_renderer->render(&p, widget_r);
	static constexpr int ADJ = 2;
	if(m_mouseInConvRect) {
		QColor c(Qt::cyan);
		c.setAlphaF(0.2);
		p.fillRect(svgRectToWidgetRect(m_convRect).adjusted(ADJ, ADJ, -ADJ, -ADJ), c);
	}
	if(m_mouseInOmpagRect) {
		QColor c(Qt::cyan);
		c.setAlphaF(0.2);
		p.fillRect(svgRectToWidgetRect(m_ompagRect).adjusted(ADJ, ADJ, -ADJ, -ADJ), c);
	}
}

QRect VisuWidget::svgRectToWidgetRect(const QRectF &svg_rect)
{
	QSizeF svg_sz = m_renderer->defaultSize();
	QSizeF widget_sz = svg_sz.scaled(geometry().size(), Qt::KeepAspectRatio);
	QRect ret;
	ret.setX(static_cast<int>(svg_rect.x() * widget_sz.width() / svg_sz.width()));
	ret.setY(static_cast<int>(svg_rect.y() * widget_sz.height() / svg_sz.height()));
	ret.setWidth(static_cast<int>(svg_rect.width() * widget_sz.width() / svg_sz.width()));
	ret.setHeight(static_cast<int>(svg_rect.height() * widget_sz.height() / svg_sz.height()));
	return ret;
}

void VisuWidget::contextMenuEvent(QContextMenuEvent *event)
{
	const QPoint pos = event->pos();
	QMenu menu(this);

	BfsViewApp *app = BfsViewApp::instance();

	//connect(&menu, &QMenu::destroyed, []() { shvInfo() << "RIP"; });
	shvDebug() << pos.x() << pos.y();
	if(svgRectToWidgetRect(m_ompagRect).contains(pos)) {
		BfsViewApp::SwitchStatus st = app->ompagSwitchStatus();
		BfsViewApp::SwitchStatus rq_st = app->ompagRequiredSwitchStatus();
		if(st == BfsViewApp::SwitchStatus::On || rq_st != BfsViewApp::SwitchStatus::Unknown) {
			menu.addAction(tr("Vypnout"), []() {
				shvInfo() << "set ompag OFF";
				BfsViewApp::instance()->setOmpag(false);
			});
		}
		if(st == BfsViewApp::SwitchStatus::Off || rq_st != BfsViewApp::SwitchStatus::Unknown) {
			menu.addAction(tr("Zapnout"), []() {
				shvInfo() << "set ompag ON";
				BfsViewApp::instance()->setOmpag(true);
			});
		}
#ifdef Q_OS_LINUXXXX
		menu.addAction(tr("Zapnout sim"), [this]() {
			bool val = true;
			BfsViewApp *app = BfsViewApp::instance();
			int s = app->bfsStatus();
			BfsViewApp::setBit(s, BfsViewApp::BfsStatus::OmpagOn, val);
			BfsViewApp::setBit(s, BfsViewApp::BfsStatus::OmpagOff, !val);
			app->setBfsStatus(s);
		});
		menu.addAction(tr("Vypnout sim"), [this]() {
			bool val = false;
			BfsViewApp *app = BfsViewApp::instance();
			int s = app->bfsStatus();
			BfsViewApp::setBit(s, BfsViewApp::BfsStatus::OmpagOn, val);
			BfsViewApp::setBit(s, BfsViewApp::BfsStatus::OmpagOff, !val);
			app->setBfsStatus(s);
		});
#endif
	}
	else if(svgRectToWidgetRect(m_convRect).contains(pos)) {
		BfsViewApp::SwitchStatus st = app->convSwitchStatus();
		BfsViewApp::SwitchStatus rq_st = app->convRequiredSwitchStatus();
		if(st == BfsViewApp::SwitchStatus::On || rq_st != BfsViewApp::SwitchStatus::Unknown) {
			menu.addAction(tr("Vypnout BS"), []() {
				shvInfo() << "set conv OFF";
				BfsViewApp::instance()->setConv(false);
			});
		}
		if(st == BfsViewApp::SwitchStatus::Off || rq_st != BfsViewApp::SwitchStatus::Unknown) {
			menu.addAction(tr("Zapnout BS"), []() {
				shvInfo() << "set conv ON";
				BfsViewApp::instance()->setConv(true);
			});
		}
	}
	else {
		menu.addAction(tr("Nastavení"), [this]() {
			shvInfo() << "settings";
			SettingsDialog dlg(this);
			if(dlg.exec()) {
				BfsViewApp::instance()->loadSettings();
			}
		});
		menu.addAction(tr("O Aplikaci"), [this]() {
			shvInfo() << "about";
			QMessageBox::about(this
							   , tr("O Aplikaci")
							   , tr("<b>BFS View</b><br/>"
									"<br/>"
									"ver: %1<br/>"
									"Elektroline a.s. (2018)"
									).arg(QCoreApplication::applicationVersion()));
		});
	}
	if(menu.actions().count())
		menu.exec(mapToGlobal(pos));
}

void VisuWidget::mouseMoveEvent(QMouseEvent *event)
{
	bool prev_in_conv = m_mouseInConvRect;
	bool prev_in_ompag = m_mouseInOmpagRect;
	const QPoint pos = event->pos();
	m_mouseInOmpagRect = (svgRectToWidgetRect(m_ompagRect).contains(pos));
	m_mouseInConvRect = (svgRectToWidgetRect(m_convRect).contains(pos));
	if(m_mouseInConvRect != prev_in_conv) {
		shvDebug() << "mouse in conv:" << m_mouseInConvRect;
		update();
	}
	if(m_mouseInOmpagRect != prev_in_ompag) {
		shvDebug() << "mouse in ompag:" << m_mouseInOmpagRect;
		update();
	}
	Super::mouseMoveEvent(event);
}

static QString switchStatusToColor(BfsViewApp::SwitchStatus status)
{
	if(!BfsViewApp::instance()->isPlcConnected())
		return QStringLiteral("red");
	switch (status) {
	case BfsViewApp::SwitchStatus::On: return QStringLiteral("#00FF00");
	case BfsViewApp::SwitchStatus::Off: return QStringLiteral("white");
	default: return QStringLiteral("gray");
	}
}
/*
static QString bitToColor(bool bit)
{
	if(!BfsViewApp::instance()->isPlcConnected())
		return QStringLiteral("gray");
	return bit? QStringLiteral("#00FF00"): QStringLiteral("#FF0000");
}
*/
static QString goodBitToColor(bool bit)
{
	if(!BfsViewApp::instance()->isPlcConnected())
		return QStringLiteral("gray");
	return bit? QStringLiteral("#00FF00"): QStringLiteral("white");
}

static QString badBitToColor(bool bit)
{
	if(!BfsViewApp::instance()->isPlcConnected())
		return QStringLiteral("gray");
	return bit? QStringLiteral("#FF0000"): !BfsViewApp::instance()->isPlcConnected()? QStringLiteral("gray"): QStringLiteral("white");
}

static QString plcNotConnectedBitToColor(bool bit)
{
	return bit? QStringLiteral("#FF0000"): QStringLiteral("white");
}

static QDomElement elementById_helper(const QString &id, QDomElement parent)
{
	for(QDomElement el = parent.firstChildElement(); !el.isNull(); el = el.nextSiblingElement()) {
		//shvInfo() << parent.tagName() << el.tagName() << el.attribute("id") << "vs" << id;
		if(el.attribute(QStringLiteral("id")) == id)
			return el;
		QDomElement el1 = elementById_helper(id, el);
		if(!el1.isNull())
			return el1;
	}
	return QDomElement();
}

QDomElement VisuWidget::elementById(const QString &id)
{
	QDomElement el = elementById_helper(id, m_xDoc.documentElement());
	if(el.isNull()) {
		shvError() << "Element id:" << id << "does not exist";
	}
	return el;
}

static QDomElement elementByShvName_helper(const QDomElement &parent_el, const QString &shv_name)
{
	for(QDomElement el = parent_el.firstChildElement(); !el.isNull(); el = el.nextSiblingElement()) {
		//shvInfo() << parent.tagName() << el.tagName() << el.attribute("id") << "vs" << id;
		if(el.attribute(QStringLiteral("shvName")) == shv_name) {
			return el;
		}
		QDomElement el1 = elementByShvName_helper(el, shv_name);
		if(!el1.isNull()) {
			return el1;
		}
	}
	return QDomElement();
}

QDomElement VisuWidget::elementByShvName(const QDomElement &parent_el, const QString &shv_name)
{
	QDomElement ret = elementByShvName_helper(parent_el, shv_name);
	if(ret.isNull()) {
		shvError() << "Element shvName:" << shv_name << "does not exist";
	}
	return ret;
}

void VisuWidget::setElementText(const QString &elem_id, const QString &text)
{
	QDomElement el = elementById(elem_id);
	if(el.isNull())
		return;
	QDomText txt = el.firstChild().toText();
	if(txt.isNull()) {
		txt = m_xDoc.createTextNode(text);
		el.appendChild(txt);
	}
	else {
		txt.setData(text);
	}
}

void VisuWidget::setElementFillColor(const QString &id, const QString &c)
{
	setElementFillColor(elementById(id), c);
}

void VisuWidget::setElementFillColor(QDomElement el, const QString &c)
{
	setElementStyleAttribute(el, QStringLiteral("fill"), c);
}

void VisuWidget::setElementColor(const QString &id, const QString &c)
{
	setElementStyleAttribute(id, QStringLiteral("color"), c);
}

void VisuWidget::setElementVisible(QDomElement el, bool on)
{
	if(el.isNull())
		return;
	static const auto VISIBILITY = QStringLiteral("visibility");
	el.setAttribute(VISIBILITY, on? QStringLiteral("visible"): QStringLiteral("hidden"));
}

void VisuWidget::setElementVisible(const QString &elem_id, bool on)
{
	//setElementStyleAttribute(id, QStringLiteral("display"), on? QString(): QStringLiteral("none"));
	QDomElement el = elementById(elem_id);
	setElementVisible(el, on);
}

void VisuWidget::setElementStyleAttribute(const QString &id, const QString &key, const QString &val)
{
	setElementStyleAttribute(elementById(id), key, val);
}

void VisuWidget::setElementStyleAttribute(QDomElement el, const QString &key, const QString &val)
{
	if(el.isNull())
		return;

	static const auto STYLE = QStringLiteral("style");
	QString style = el.attribute(STYLE);
	QStringList sl = style.split(';');
	for (int i = 0; i < sl.count(); ++i) {
		QString &s = sl[i];
		if(s.startsWith(key) && s.length() > key.length() && s[key.length()] == ':') {
			if(val.isEmpty())
				sl.removeAt(i);
			else
				s = key + ':' + val;
			//shvInfo() << elem_id << sl.join(';');
			el.setAttribute(STYLE, sl.join(';'));
			break;
		}
	}
}

void VisuWidget::refreshVisualization()
{
	shvLogFuncFrame();

	using SwitchStatus = BfsViewApp::SwitchStatus;
	using BfsStatus = BfsViewApp::BfsStatus;
	using PwrStatus = BfsViewApp::PwrStatus;

	BfsViewApp *app = BfsViewApp::instance();

	unsigned ps = app->bfsStatus();
	BfsViewApp::SwitchStatus bfs_status = static_cast<BfsViewApp::SwitchStatus>((ps & ((1 << static_cast<int>(BfsViewApp::BfsStatus::BfsOn)) | (1 << static_cast<int>(BfsViewApp::BfsStatus::BfsOff)))) >> static_cast<int>(BfsViewApp::BfsStatus::BfsOn));
	//shvInfo() << ps << bfs_status;

	m_ompagVisuController->updateXml();
	m_convVisuController->updateXml();

	setElementFillColor(QStringLiteral("shv_rect_bfsPower"), goodBitToColor(bfs_status == SwitchStatus::On));

	setElementFillColor(QStringLiteral("shv_rect_power"), (app->pwrStatus() == PwrStatus::On)? QStringLiteral("#00FF00"): QStringLiteral("red"));

	shvDebug() << "app->rpcConnection()->isBrokerConnected():" << app->rpcConnection()->isBrokerConnected();
	if(app->rpcConnection()->isBrokerConnected()) {
		setElementVisible(QStringLiteral("shv_rect_error"), BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Warning) || BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Error));
		setElementVisible(QStringLiteral("shv_txt_error"), BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Warning) || BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Error));
		setElementFillColor(QStringLiteral("shv_rect_error"),
							BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Error)? "#FF0000"
							: BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Warning)? "yellow"
							: "none");
		setElementFillColor(QStringLiteral("shv_txt_error"),
							BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Error)? "white"
							: BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Warning)? "black"
							: "none");
		setElementText(QStringLiteral("shv_tspan_error"),
							BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Error)? tr("Porucha")
							: BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Warning)? tr("Výstraha")
							: QString());
	}
	else {
		setElementVisible(QStringLiteral("shv_rect_error"), true);
		setElementVisible(QStringLiteral("shv_txt_error"), true);
		setElementFillColor(QStringLiteral("shv_rect_error"), "lightgray");
		setElementFillColor(QStringLiteral("shv_txt_error"), "black");
		setElementText(QStringLiteral("shv_tspan_error"), tr("Připojování"));
	}

	setElementFillColor(QStringLiteral("shv_rect_buffering"), goodBitToColor(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Buffering)));
	setElementFillColor(QStringLiteral("shv_rect_charging"), goodBitToColor(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Charging)));

	setElementFillColor(QStringLiteral("shv_rect_cooling"), goodBitToColor(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Cooling)));
	setElementFillColor(QStringLiteral("shv_rect_shvDisconnected"), plcNotConnectedBitToColor(
							BfsViewApp::isBit(app->bfsStatus(), BfsStatus::ShvDisconnected)
							|| !app->isPlcConnected()
							));
	shvMessage() << "BFS status:" << app->bfsStatus();
	if(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::BfsOn)) shvMessage() << static_cast<int>(BfsStatus::BfsOn) << "BfsOn";
	if(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::BfsOff)) shvMessage() << static_cast<int>(BfsStatus::BfsOff) << "BfsOff";
	if(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Buffering)) shvMessage() << static_cast<int>(BfsStatus::Buffering) << "Buffering";
	if(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Charging)) shvMessage() << static_cast<int>(BfsStatus::Charging) << "Charging";
	if(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::ConvOn)) shvMessage() << static_cast<int>(BfsStatus::ConvOn) << "ConvOn";
	if(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::OmpagOn)) shvMessage() << static_cast<int>(BfsStatus::OmpagOn) << "OmpagOn";
	if(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::OmpagOff)) shvMessage() << static_cast<int>(BfsStatus::OmpagOff) << "OmpagOff";
	if(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Cooling)) shvMessage() << static_cast<int>(BfsStatus::Cooling) << "Cooling";
	if(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Error)) shvMessage() << static_cast<int>(BfsStatus::Error) << "Error";
	if(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::Warning)) shvMessage() << static_cast<int>(BfsStatus::Warning) << "Warning";
	if(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::ShvDisconnected)) shvMessage() << static_cast<int>(BfsStatus::ShvDisconnected) << "ShvDisconnected";
	if(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::MswOn)) shvMessage() << static_cast<int>(BfsStatus::MswOn) << "MswOn";
	if(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::MswOff)) shvMessage() << static_cast<int>(BfsStatus::MswOff) << "MswOff";
	if(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::HsswTOn)) shvMessage() << static_cast<int>(BfsStatus::HsswTOn) << "HsswTOn";
	if(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::DoorOpen)) shvMessage() << static_cast<int>(BfsStatus::DoorOpen) << "DoorOpen";
	if(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::FswOn)) shvMessage() << static_cast<int>(BfsStatus::FswOn) << "FswOn";

	setElementFillColor(QStringLiteral("shv_rect_doorOpen"), badBitToColor(BfsViewApp::isBit(app->bfsStatus(), BfsStatus::DoorOpen)));
	setElementFillColor(QStringLiteral("shv_rect_fswOn"), badBitToColor(!BfsViewApp::isBit(app->bfsStatus(), BfsStatus::FswOn)));

	QByteArray ba = m_xDoc.toByteArray(0);
	//shvInfo() << m_xDoc.toString();
	load(ba);
}


SwitchVisuController::SwitchVisuController(const QString &xml_id, VisuWidget *vw, std::function<BfsViewApp::SwitchStatus (BfsViewApp*)> sfn)
	: m_xmlId(xml_id)
	, m_visuWidget(vw)
	, m_statusFn(sfn)
{

}

void SwitchVisuController::updateXml()
{
	BfsViewApp *app = BfsViewApp::instance();
	BfsViewApp::SwitchStatus status = m_statusFn(app);
	//if(status == m_requiredSwitchStatus) {
	//	m_requiredSwitchStatus = BfsViewApp::SwitchStatus::Unknown;
	//}
	QDomElement el_ctl = m_visuWidget->elementById(m_xmlId);
	//shvInfo() << "status ->" << status << statusToColor(status);
	m_visuWidget->setElementFillColor(m_visuWidget->elementByShvName(el_ctl, QStringLiteral("sw_rect")), statusToColor(status));
	m_visuWidget->setElementVisible(m_visuWidget->elementByShvName(el_ctl, QStringLiteral("sw_on")), status == BfsViewApp::SwitchStatus::On);
	m_visuWidget->setElementVisible(m_visuWidget->elementByShvName(el_ctl, QStringLiteral("sw_off")), status == BfsViewApp::SwitchStatus::Off);
}

void SwitchVisuController::onRequiredSwitchStatusChanged(BfsViewApp::SwitchStatus st)
{
	BfsViewApp *app = BfsViewApp::instance();
	BfsViewApp::SwitchStatus status = m_statusFn(app);
	m_requiredSwitchStatus = st;
	if(m_requiredSwitchStatus == status) {
		if(m_blinkTimer)
			m_blinkTimer->stop();
		m_visuWidget->refreshVisualization();
	}
	else {
		//m_requiredStatus = st;
		if(!m_blinkTimer) {
			m_blinkTimer = new QTimer(this);
			connect(m_blinkTimer, &QTimer::timeout, [this]() {
				m_toggleBit = !m_toggleBit;
				m_visuWidget->refreshVisualization();
			});
		}
		m_blinkTimer->start(500);
	}
}

QString SwitchVisuController::statusToColor(BfsViewApp::SwitchStatus status_arg)
{
	if(m_requiredSwitchStatus != BfsViewApp::SwitchStatus::Unknown) {
		BfsViewApp *app = BfsViewApp::instance();
		BfsViewApp::SwitchStatus status = m_statusFn(app);
		if(status != m_requiredSwitchStatus) {
			if(m_toggleBit)
				return QStringLiteral("orangered");
		}
	}
	return ::switchStatusToColor(status_arg);
}
