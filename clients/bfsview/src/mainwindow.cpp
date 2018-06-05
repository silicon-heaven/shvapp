#include "bfsviewapp.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/deviceconnection.h>

#include <QFile>

#define TEST

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m_xDoc("SVG")
{
	ui->setupUi(this);
#ifdef TEST
	connect(BfsViewApp::instance(), &BfsViewApp::ompagStatusChanged, this, &MainWindow::refreshStatus);
	connect(BfsViewApp::instance(), &BfsViewApp::bsStatusChanged, this, &MainWindow::refreshStatus);
#endif
	connect(BfsViewApp::instance(), &BfsViewApp::bfsStatusChanged, this, &MainWindow::refreshStatus);
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
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::on_actionOmpagOn_toggled(bool on)
{
	BfsViewApp::instance()->setOmpagStatus(on? (int)BfsViewApp::SwitchStatus::On : (int)BfsViewApp::SwitchStatus::Off);
}

void MainWindow::on_actionBsOn_toggled(bool on)
{
	BfsViewApp::instance()->setBsStatus(on? (int)BfsViewApp::SwitchStatus::On : (int)BfsViewApp::SwitchStatus::Off);
}

void MainWindow::on_actionPwrStatus_toggled(bool on)
{
	BfsViewApp::instance()->setPwrStatus(on? 1: 0);
}

static QString statusToColor(int status)
{
	switch ((BfsViewApp::SwitchStatus)status) {
	case BfsViewApp::SwitchStatus::On: return QStringLiteral("lime");
	case BfsViewApp::SwitchStatus::Off: return QStringLiteral("red");
	default: return QStringLiteral("gray");
	}
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

void MainWindow::setElementFillColor(const QString &id, const QString &c)
{
	setElementStyleAttribute(id, QStringLiteral("fill"), c);
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
#ifdef TEST
	int ompag_status = app->ompagStatus();
	int bs_status = app->bsStatus();
#else
	unsigned ps = app->bfsStatus();
	int ompag_status = (ps & ((1 << BfsViewApp::BfsStatus::OmpagOn) | (1 << BfsViewApp::BfsStatus::OmpagOff))) >> BfsViewApp::BfsStatus::OmpagOn;
	int bs_status = (ps & ((1 << BfsViewApp::BfsStatus::MswOn) | (1 << BfsViewApp::BfsStatus::MswOff))) >> BfsViewApp::BfsStatus::MswOn;
#endif
	setElementFillColor(QStringLiteral("sw_ompag_rect"), statusToColor(ompag_status));
	setElementVisible(QStringLiteral("sw_ompag_on"), ompag_status == (int)BfsViewApp::SwitchStatus::On);
	setElementVisible(QStringLiteral("sw_ompag_off"), ompag_status == (int)BfsViewApp::SwitchStatus::Off);

	setElementFillColor(QStringLiteral("sw_bs_rect"), statusToColor(bs_status));
	setElementVisible(QStringLiteral("sw_bs_on"), bs_status == (int)BfsViewApp::SwitchStatus::On);
	setElementVisible(QStringLiteral("sw_bs_off"), bs_status == (int)BfsViewApp::SwitchStatus::Off);

	setElementFillColor(QStringLiteral("shv_rect_bfs"), app->rpcConnection()->isBrokerConnected()? "white": "salmon");

	QByteArray ba = m_xDoc.toByteArray(0);
	//shvInfo() << m_xDoc.toString();
	ui->visuWidget->load(ba);
}


