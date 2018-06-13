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

static QString r2s(const QRectF &r)
{
	return QString("(x: %1 y %2 w %3 h %4)").arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
}

VisuWidget::VisuWidget(QWidget *parent)
	: Super(parent)
	, m_renderer(new QSvgRenderer(this))
	, m_xDoc("SVG")
{
	//setContextMenuPolicy(Qt::CustomContextMenu);
	{
		QFile file(":/images/bfs1.svg");
		if (!file.open(QIODevice::ReadOnly))
			return;
		if (!m_xDoc.setContent(&file)) {
			shvError() << "Error parsing SVG file";
			QCoreApplication::instance()->quit();
		}
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
	}
}

bool VisuWidget::load(const QByteArray &svg)
{
	bool ret = m_renderer->load(svg);
	if(ret)
		update();
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
	/*
	QRect r2;
	r2.setX(m_ompagRect.x() * widget_r.width() / svg_sz.width());
	r2.setY(m_ompagRect.y() * widget_r.height() / svg_sz.height());
	r2.setWidth(m_ompagRect.width() * widget_r.width() / svg_sz.width());
	r2.setHeight(m_ompagRect.height() * widget_r.height() / svg_sz.height());
	shvDebug() << "svg:" << r2s(QRect({0, 0}, svg_sz));
	shvDebug() << "r:" << r2s(widget_r);
	shvDebug() << "r_ompag:" << r2s(m_ompagRect);
	shvDebug() << "r2:" << r2s(r2);
	p.fillRect(r2, QColor("khaki"));
	p.fillRect(svgRectToWidgetRect(m_convRect), QColor("salmon"));
	*/
}

QRect VisuWidget::svgRectToWidgetRect(const QRectF &svg_rect)
{
	QSizeF svg_sz = m_renderer->defaultSize();
	QSizeF widget_sz = svg_sz.scaled(geometry().size(), Qt::KeepAspectRatio);
	QRect ret;
	ret.setX(svg_rect.x() * widget_sz.width() / svg_sz.width());
	ret.setY(svg_rect.y() * widget_sz.height() / svg_sz.height());
	ret.setWidth(svg_rect.width() * widget_sz.width() / svg_sz.width());
	ret.setHeight(svg_rect.height() * widget_sz.height() / svg_sz.height());
	return ret;
}

int VisuWidget::ompagStatus()
{
	unsigned ps = BfsViewApp::instance()->bfsStatus();
	int status = (ps & ((1 << BfsViewApp::BfsStatus::OmpagOn) | (1 << BfsViewApp::BfsStatus::OmpagOff))) >> BfsViewApp::BfsStatus::OmpagOn;
	return status;
}

int VisuWidget::mswStatus()
{
	unsigned ps = BfsViewApp::instance()->bfsStatus();
	int status = (ps & ((1 << BfsViewApp::BfsStatus::MswOn) | (1 << BfsViewApp::BfsStatus::MswOff))) >> BfsViewApp::BfsStatus::MswOn;
	return status;
}

void VisuWidget::contextMenuEvent(QContextMenuEvent *event)
{
	const QPoint pos = event->pos();
	QMenu menu(this);
	//connect(&menu, &QMenu::destroyed, []() { shvInfo() << "RIP"; });
	shvDebug() << pos.x() << pos.y();
	if(svgRectToWidgetRect(m_ompagRect).contains(pos)) {
		int st = ompagStatus();
		if(st == BfsViewApp::SwitchStatus::On) {
			menu.addAction(tr("Vypnout"), [this]() {
				shvInfo() << "set ompag OFF";
				BfsViewApp::instance()->setOmpag(false);
			});
		}
		else if(st == BfsViewApp::SwitchStatus::Off) {
			menu.addAction(tr("Zapnout"), [this]() {
				shvInfo() << "set ompag ON";
				BfsViewApp::instance()->setOmpag(true);
			});
		}
	}
	else if(svgRectToWidgetRect(m_convRect).contains(pos)) {
		int st = mswStatus();
		if(st == BfsViewApp::SwitchStatus::On) {
			menu.addAction(tr("Vypnout BS"), [this]() {
				shvInfo() << "set conv OFF";
				BfsViewApp::instance()->setConv(false);
			});
		}
		else if(st == BfsViewApp::SwitchStatus::Off) {
			menu.addAction(tr("Zapnout BS"), [this]() {
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

static QString statusToColor(int status)
{
	if(!BfsViewApp::instance()->isPlcConnected())
		return QStringLiteral("red");
	switch ((BfsViewApp::SwitchStatus)status) {
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

QDomElement elementById_helper(const QString &id, QDomElement parent)
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
	setElementStyleAttribute(id, QStringLiteral("fill"), c);
}

void VisuWidget::setElementColor(const QString &id, const QString &c)
{
	setElementStyleAttribute(id, QStringLiteral("color"), c);
}

void VisuWidget::setElementVisible(const QString &elem_id, bool on)
{
	//setElementStyleAttribute(id, QStringLiteral("display"), on? QString(): QStringLiteral("none"));
	QDomElement el = elementById(elem_id);
	if(el.isNull())
		return;

	static const auto VISIBILITY = QStringLiteral("visibility");
	el.setAttribute(VISIBILITY, on? QStringLiteral("visible"): QStringLiteral("hidden"));
}

void VisuWidget::setElementStyleAttribute(const QString &elem_id, const QString &key, const QString &val)
{
	QDomElement el = elementById(elem_id);
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
	BfsViewApp *app = BfsViewApp::instance();

	unsigned ps = app->bfsStatus();
	int ompag_status = ompagStatus();
	int msw_status = mswStatus();
	int bfs_status = (ps & ((1 << BfsViewApp::BfsStatus::BfsOn) | (1 << BfsViewApp::BfsStatus::BfsOff))) >> BfsViewApp::BfsStatus::BfsOn;
	//bool is_plc_connected = app->isPlcConnected();
	shvInfo() << ps << bfs_status;

	setElementFillColor(QStringLiteral("sw_ompag_rect"), statusToColor(ompag_status));
	setElementVisible(QStringLiteral("sw_ompag_on"), ompag_status == (int)BfsViewApp::SwitchStatus::On);
	setElementVisible(QStringLiteral("sw_ompag_off"), ompag_status == (int)BfsViewApp::SwitchStatus::Off);

	setElementFillColor(QStringLiteral("sw_bs_rect"), statusToColor(msw_status));
	setElementVisible(QStringLiteral("sw_bs_on"), msw_status == (int)BfsViewApp::SwitchStatus::On);
	setElementVisible(QStringLiteral("sw_bs_off"), msw_status == (int)BfsViewApp::SwitchStatus::Off);

	setElementFillColor(QStringLiteral("shv_rect_bfsPower"), goodBitToColor(bfs_status == BfsViewApp::SwitchStatus::On));

	setElementFillColor(QStringLiteral("shv_rect_power"), app->pwrStatus()? QStringLiteral("#00FF00"): QStringLiteral("red"));

	shvDebug() << "app->rpcConnection()->isBrokerConnected():" << app->rpcConnection()->isBrokerConnected();
	if(app->rpcConnection()->isBrokerConnected()) {
		setElementVisible(QStringLiteral("shv_rect_error"), BfsViewApp::isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Warning) || BfsViewApp::isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Error));
		setElementVisible(QStringLiteral("shv_txt_error"), BfsViewApp::isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Warning) || BfsViewApp::isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Error));
		setElementFillColor(QStringLiteral("shv_rect_error"),
							BfsViewApp::isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Error)? "#FF0000"
							: BfsViewApp::isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Warning)? "yellow"
							: "none");
		setElementFillColor(QStringLiteral("shv_txt_error"),
							BfsViewApp::isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Error)? "white"
							: BfsViewApp::isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Warning)? "black"
							: "none");
		setElementText(QStringLiteral("shv_tspan_error"),
							BfsViewApp::isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Error)? tr("Porucha")
							: BfsViewApp::isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Warning)? tr("Výstraha")
							: QString());
	}
	else {
		setElementVisible(QStringLiteral("shv_rect_error"), true);
		setElementVisible(QStringLiteral("shv_txt_error"), true);
		setElementFillColor(QStringLiteral("shv_rect_error"), "lightgray");
		setElementFillColor(QStringLiteral("shv_txt_error"), "black");
		setElementText(QStringLiteral("shv_tspan_error"), tr("Připojování"));
	}

	setElementFillColor(QStringLiteral("shv_rect_buffering"), goodBitToColor(BfsViewApp::isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Buffering)));
	setElementFillColor(QStringLiteral("shv_rect_charging"), goodBitToColor(BfsViewApp::isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Charging)));

	setElementFillColor(QStringLiteral("shv_rect_cooling"), goodBitToColor(BfsViewApp::isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Cooling)));
	setElementFillColor(QStringLiteral("shv_rect_shvDisconnected"), plcNotConnectedBitToColor(
							BfsViewApp::isBit(app->bfsStatus(), BfsViewApp::BfsStatus::ShvDisconnected)
							|| !app->isPlcConnected()
							));
	setElementFillColor(QStringLiteral("shv_rect_doorOpen"), badBitToColor(BfsViewApp::isBit(app->bfsStatus(), BfsViewApp::BfsStatus::DoorOpen)));
	setElementFillColor(QStringLiteral("shv_rect_fswOn"), badBitToColor(!BfsViewApp::isBit(app->bfsStatus(), BfsViewApp::BfsStatus::FswOn)));

	QByteArray ba = m_xDoc.toByteArray(0);
	//shvInfo() << m_xDoc.toString();
	load(ba);
}

