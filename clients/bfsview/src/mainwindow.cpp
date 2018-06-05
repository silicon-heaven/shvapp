#include "bfsviewapp.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/deviceconnection.h>

#include <QFile>
#include <QSettings>
#include <QTimer>

static void setBit(int &val, int bit_no, bool b)
{
	int mask = 1 << bit_no;
	if(b)
		val |= mask;
	else
		val &= ~mask;
}

static bool isBit(int val, int bit_no)
{
	int mask = 1 << bit_no;
	return val & mask;
}

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m_xDoc("SVG")
{
	ui->setupUi(this);
#ifdef TEST
	connect(ui->bfsPower, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		setBit(s, BfsViewApp::BfsStatus::BfsOn, on);
		setBit(s, BfsViewApp::BfsStatus::BfsOff, !on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
	connect(ui->buffering, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		setBit(s, BfsViewApp::BfsStatus::Buffering, on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
	connect(ui->charging, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		setBit(s, BfsViewApp::BfsStatus::Charging, on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
	connect(ui->cooling, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		setBit(s, BfsViewApp::BfsStatus::Cooling, on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
	connect(ui->shvDisconnected, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		setBit(s, BfsViewApp::BfsStatus::ShvDisconnected, on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
	connect(ui->doorOpen, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		setBit(s, BfsViewApp::BfsStatus::DoorOpen, on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
	connect(ui->fswOn, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		setBit(s, BfsViewApp::BfsStatus::FswOn, on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
	connect(ui->Error, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		setBit(s, BfsViewApp::BfsStatus::Error, on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
	connect(ui->Warning, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		setBit(s, BfsViewApp::BfsStatus::Warning, on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
#else
	ui->frmTest->hide();
#endif
	connect(BfsViewApp::instance(), &BfsViewApp::bfsStatusChanged, this, &MainWindow::refreshStatus);
	connect(BfsViewApp::instance(), &BfsViewApp::pwrStatusChanged, this, &MainWindow::refreshStatus);
	connect(BfsViewApp::instance()->rpcConnection(), &shv::iotqt::rpc::DeviceConnection::brokerConnectedChanged, this, &MainWindow::refreshStatus);

	{
		QFile file(":/images/bfs1.svg");
		if (!file.open(QIODevice::ReadOnly))
			return;
		if (!m_xDoc.setContent(&file)) {
			shvError() << "Error parsing SVG file";
			QCoreApplication::instance()->quit();
		}
	}
	refreshStatus();

	QTimer::singleShot(0, [this]() {
		QSettings settings;
		restoreGeometry(settings.value("geometry").toByteArray());
	});
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::on_ompagOn_toggled(bool on)
{
#ifdef TEST
	int s = BfsViewApp::instance()->bfsStatus();
	setBit(s, BfsViewApp::BfsStatus::OmpagOn, on);
	setBit(s, BfsViewApp::BfsStatus::OmpagOff, !on);
	BfsViewApp::instance()->setBfsStatus(s);
#else
	BfsViewApp::instance()->setOmpag(on);
#endif
}

void MainWindow::on_convOn_toggled(bool on)
{
#ifdef TEST
	int s = BfsViewApp::instance()->bfsStatus();
	setBit(s, BfsViewApp::BfsStatus::MswOn, on);
	setBit(s, BfsViewApp::BfsStatus::MswOff, !on);
	BfsViewApp::instance()->setBfsStatus(s);
#else
	BfsViewApp::instance()->setConv(on);
#endif
}

void MainWindow::on_pwrOn_toggled(bool on)
{
	BfsViewApp::instance()->setPwrStatus(on? 1: 0);
}

static QString statusToColor(int status)
{
	switch ((BfsViewApp::SwitchStatus)status) {
	case BfsViewApp::SwitchStatus::On: return QStringLiteral("#00FF00");
	case BfsViewApp::SwitchStatus::Off: return QStringLiteral("#FF0000");
	default: return QStringLiteral("gray");
	}
}

static QString bitToColor(bool bit)
{
	return bit? QStringLiteral("#00FF00"): QStringLiteral("#FF0000");
}

static QString goodBitToColor(bool bit)
{
	return bit? QStringLiteral("#00FF00"): QStringLiteral("lightgray");
}

static QString badBitToColor(bool bit)
{
	return bit? QStringLiteral("#FF0000"): QStringLiteral("lightgray");
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

QDomElement MainWindow::elementById(const QString &id)
{
	QDomElement el = elementById_helper(id, m_xDoc.documentElement());
	if(el.isNull()) {
		shvError() << "Element id:" << id << "does not exist";
	}
	return el;
}

void MainWindow::setElementText(const QString &elem_id, const QString &text)
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

void MainWindow::setElementFillColor(const QString &id, const QString &c)
{
	setElementStyleAttribute(id, QStringLiteral("fill"), c);
}

void MainWindow::setElementColor(const QString &id, const QString &c)
{
	setElementStyleAttribute(id, QStringLiteral("color"), c);
}

void MainWindow::setElementVisible(const QString &elem_id, bool on)
{
	//setElementStyleAttribute(id, QStringLiteral("display"), on? QString(): QStringLiteral("none"));
	QDomElement el = elementById(elem_id);
	if(el.isNull())
		return;

	static const auto VISIBILITY = QStringLiteral("visibility");
	el.setAttribute(VISIBILITY, on? QStringLiteral("visible"): QStringLiteral("hidden"));
}

void MainWindow::setElementStyleAttribute(const QString &elem_id, const QString &key, const QString &val)
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

void MainWindow::refreshStatus()
{
	BfsViewApp *app = BfsViewApp::instance();

	unsigned ps = app->bfsStatus();
	int ompag_status = (ps & ((1 << BfsViewApp::BfsStatus::OmpagOn) | (1 << BfsViewApp::BfsStatus::OmpagOff))) >> BfsViewApp::BfsStatus::OmpagOn;
	int msw_status = (ps & ((1 << BfsViewApp::BfsStatus::MswOn) | (1 << BfsViewApp::BfsStatus::MswOff))) >> BfsViewApp::BfsStatus::MswOn;
	int bfs_status = (ps & ((1 << BfsViewApp::BfsStatus::BfsOn) | (1 << BfsViewApp::BfsStatus::BfsOff))) >> BfsViewApp::BfsStatus::BfsOn;

	//shvInfo() << ps << ompag_status << msw_status;

	setElementFillColor(QStringLiteral("sw_ompag_rect"), statusToColor(ompag_status));
	setElementVisible(QStringLiteral("sw_ompag_on"), ompag_status == (int)BfsViewApp::SwitchStatus::On);
	setElementVisible(QStringLiteral("sw_ompag_off"), ompag_status == (int)BfsViewApp::SwitchStatus::Off);

	setElementFillColor(QStringLiteral("sw_bs_rect"), statusToColor(msw_status));
	setElementVisible(QStringLiteral("sw_bs_on"), msw_status == (int)BfsViewApp::SwitchStatus::On);
	setElementVisible(QStringLiteral("sw_bs_off"), msw_status == (int)BfsViewApp::SwitchStatus::Off);

	setElementFillColor(QStringLiteral("shv_rect_bfsPower"), statusToColor(bfs_status));

	setElementFillColor(QStringLiteral("shv_rect_bfs"), app->rpcConnection()->isBrokerConnected()? "white": "salmon");

	setElementFillColor(QStringLiteral("shv_rect_power"), bitToColor(app->pwrStatus()));

	setElementVisible(QStringLiteral("shv_rect_error"), isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Warning) || isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Error));
	setElementVisible(QStringLiteral("shv_txt_error"), isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Warning) || isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Error));
	setElementFillColor(QStringLiteral("shv_rect_error"),
						isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Error)? "#FF0000"
						: isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Warning)? "yellow"
						: "none");
	setElementFillColor(QStringLiteral("shv_txt_error"),
						isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Error)? "white"
						: isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Warning)? "black"
						: "none");
	setElementText(QStringLiteral("shv_tspan_error"),
						isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Error)? "Porucha"
						: isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Warning)? "Varování"
						: QString());

	setElementFillColor(QStringLiteral("shv_rect_buffering"), goodBitToColor(isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Buffering)));
	setElementFillColor(QStringLiteral("shv_rect_charging"), goodBitToColor(isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Charging)));

	setElementFillColor(QStringLiteral("shv_rect_cooling"), goodBitToColor(isBit(app->bfsStatus(), BfsViewApp::BfsStatus::Cooling)));
	setElementFillColor(QStringLiteral("shv_rect_shvDisconnected"), badBitToColor(isBit(app->bfsStatus(), BfsViewApp::BfsStatus::ShvDisconnected)));
	setElementFillColor(QStringLiteral("shv_rect_doorOpen"), badBitToColor(isBit(app->bfsStatus(), BfsViewApp::BfsStatus::DoorOpen)));
	setElementFillColor(QStringLiteral("shv_rect_fswOn"), badBitToColor(!isBit(app->bfsStatus(), BfsViewApp::BfsStatus::FswOn)));

	QByteArray ba = m_xDoc.toByteArray(0);
	//shvInfo() << m_xDoc.toString();
	ui->visuWidget->load(ba);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	QSettings settings;
	settings.setValue("geometry", saveGeometry());
	Super::closeEvent(event);
}


