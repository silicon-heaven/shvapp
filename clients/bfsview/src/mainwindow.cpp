#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <shv/coreqt/log.h>

#include <QFile>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
	, m_xDoc("SVG")
{
	ui->setupUi(this);

	connect(this, &MainWindow::ompagStatusChanged, this, &MainWindow::refreshStatus);
	connect(this, &MainWindow::bsStatusChanged, this, &MainWindow::refreshStatus);

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
	setOmpagStatus(on? (int)SwitchStatus::On : (int)SwitchStatus::Off);
}

void MainWindow::on_actionBsOn_toggled(bool on)
{
	setBsStatus(on? (int)SwitchStatus::On : (int)SwitchStatus::Off);
}

static QString statusToColor(int status)
{
	switch ((MainWindow::SwitchStatus)status) {
	case MainWindow::SwitchStatus::On: return QStringLiteral("lime");
	case MainWindow::SwitchStatus::Off: return QStringLiteral("red");
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
	setElementFillColor(QStringLiteral("sw_ompag_rect"), statusToColor(ompagStatus()));
	setElementVisible(QStringLiteral("sw_ompag_on"), ompagStatus() == (int)SwitchStatus::On);
	setElementVisible(QStringLiteral("sw_ompag_off"), ompagStatus() <= (int)SwitchStatus::Off);

	setElementFillColor(QStringLiteral("sw_bs_rect"), statusToColor(bsStatus()));
	setElementVisible(QStringLiteral("sw_bs_on"), bsStatus() == (int)SwitchStatus::On);
	setElementVisible(QStringLiteral("sw_bs_off"), bsStatus() <= (int)SwitchStatus::Off);

	QByteArray ba = m_xDoc.toByteArray(0);
	//shvInfo() << m_xDoc.toString();
	ui->visuWidget->load(ba);
}

