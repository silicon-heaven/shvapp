#include "bfsviewapp.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/deviceconnection.h>

#include <QFile>
#include <QSettings>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	setWindowIcon(QIcon(":/images/eline"));
	setWindowTitle(tr("BFS View"));

#ifdef TEST
	connect(ui->bfsPower, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		BfsViewApp::setBit(s, BfsViewApp::BfsStatus::BfsOn, on);
		BfsViewApp::setBit(s, BfsViewApp::BfsStatus::BfsOff, !on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
	connect(ui->buffering, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		BfsViewApp::setBit(s, BfsViewApp::BfsStatus::Buffering, on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
	connect(ui->charging, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		BfsViewApp::setBit(s, BfsViewApp::BfsStatus::Charging, on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
	connect(ui->cooling, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		BfsViewApp::setBit(s, BfsViewApp::BfsStatus::Cooling, on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
	connect(ui->shvDisconnected, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		BfsViewApp::setBit(s, BfsViewApp::BfsStatus::ShvDisconnected, on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
	connect(ui->doorOpen, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		BfsViewApp::setBit(s, BfsViewApp::BfsStatus::DoorOpen, on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
	connect(ui->fswOn, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		BfsViewApp::setBit(s, BfsViewApp::BfsStatus::FswOn, on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
	connect(ui->Error, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		BfsViewApp::setBit(s, BfsViewApp::BfsStatus::Error, on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
	connect(ui->Warning, &QCheckBox::toggled, [this](bool on) {
		int s = BfsViewApp::instance()->bfsStatus();
		BfsViewApp::setBit(s, BfsViewApp::BfsStatus::Warning, on);
		BfsViewApp::instance()->setBfsStatus(s);
	});
#else
	ui->frmTest->hide();
#endif
	connect(BfsViewApp::instance(), &BfsViewApp::bfsStatusChanged, ui->visuWidget, &VisuWidget::refreshVisualization);
	connect(BfsViewApp::instance(), &BfsViewApp::pwrStatusChanged, ui->visuWidget, &VisuWidget::refreshVisualization);
	connect(BfsViewApp::instance()->rpcConnection(), &shv::iotqt::rpc::DeviceConnection::brokerConnectedChanged, ui->visuWidget, &VisuWidget::refreshVisualization);

	ui->visuWidget->refreshVisualization();

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
	BfsViewApp::instance()->setOmpag(on);
}

void MainWindow::on_convOn_toggled(bool on)
{
	BfsViewApp::instance()->setConv(on);
}

void MainWindow::on_pwrOn_toggled(bool on)
{
	BfsViewApp::instance()->setPwrStatus(on? 1: 0);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	QSettings settings;
	settings.setValue("geometry", saveGeometry());
	Super::closeEvent(event);
}


