#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "jn50viewapp.h"
#include "appclioptions.h"
#include "settingsdialog.h"

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
	setWindowTitle(tr("Converter JN50 View"));

#ifdef TEST
	connect(ui->oclOn, &QCheckBox::toggled, [](bool on) {
		Jn50ViewApp *app = Jn50ViewApp::instance();
		unsigned s = app->convStatus();
		s = Jn50ViewApp::setMask(s, Jn50ViewApp::ConvStatus::OCL_VOLTAGE, on);
		app->setConvStatus(s);
	});
	connect(ui->doorOpen, &QCheckBox::toggled, [](bool on) {
		Jn50ViewApp *app = Jn50ViewApp::instance();
		unsigned s = app->convStatus();
		s = Jn50ViewApp::setMask(s, Jn50ViewApp::ConvStatus::DOOR_OPEN, on);
		app->setConvStatus(s);
	});
	connect(ui->acReady, &QCheckBox::toggled, [](bool on) {
		Jn50ViewApp *app = Jn50ViewApp::instance();
		unsigned s = app->convStatus();
		s = Jn50ViewApp::setMask(s, Jn50ViewApp::ConvStatus::AC_READY, on);
		app->setConvStatus(s);
	});
	connect(ui->shvConnected, &QCheckBox::toggled, [](bool on) {
		Jn50ViewApp *app = Jn50ViewApp::instance();
		app->setShvDeviceConnected(on);
	});
	connect(ui->inI, QOverload<int>::of(&QSpinBox::valueChanged), [](int val) {
		Jn50ViewApp *app = Jn50ViewApp::instance();
		app->setShvDeviceValue("inI", val);
	});
	connect(ui->inU, QOverload<int>::of(&QSpinBox::valueChanged), [](int val) {
		Jn50ViewApp *app = Jn50ViewApp::instance();
		app->setShvDeviceValue("inU", val);
	});
#else
	ui->frmTest->hide();
#endif

	connect(ui->btRun, &QPushButton::clicked, []() {
		Jn50ViewApp *app = Jn50ViewApp::instance();
		app->rpcConnection()->callShvMethod(app->cliOptions()->converterShvPath(), "start");
	});
	connect(ui->btOff, &QPushButton::clicked, []() {
		Jn50ViewApp *app = Jn50ViewApp::instance();
		app->rpcConnection()->callShvMethod(app->cliOptions()->converterShvPath(), "stop");
	});
	connect(ui->btSettings, &QPushButton::clicked, [this]() {
		SettingsDialog dlg(this);
		dlg.exec();
	});

	QTimer::singleShot(0, [this]() {
		QSettings settings;
		restoreGeometry(settings.value("geometry").toByteArray());
	});
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
	shvDebug() << __FUNCTION__;
	Super::resizeEvent(event);
	ui->visuWidget->zoomToFitDeferred();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	QSettings settings;
	settings.setValue("geometry", saveGeometry());
	Super::closeEvent(event);
}


