#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "jn50viewapp.h"
#include "appclioptions.h"
#include "settingsdialog.h"
#include "dlgapplog.h"
#include "thresholdsdialog.h"
#include "settings.h"
#include "logview/dlglogview.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/deviceconnection.h>

#include <QDesktopServices>
#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	setWindowIcon(QIcon(":/images/eline"));
	setWindowTitle(tr("Converter JN50"));

#ifdef TESTING
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

	ui->btSwitchOn->setDefaultAction(ui->actSwitchOn);
	ui->btSwitchOff->setDefaultAction(ui->actSwitchOff);
	connect(ui->actSwitchOn, &QAction::triggered, []() {
		Jn50ViewApp *app = Jn50ViewApp::instance();
		app->rpcConnection()->callShvMethod(app->cliOptions()->converterShvPath(), "start");
	});
	connect(ui->actSwitchOff, &QAction::triggered, []() {
		Jn50ViewApp *app = Jn50ViewApp::instance();
		app->rpcConnection()->callShvMethod(app->cliOptions()->converterShvPath(), "stop");
	});
	connect(ui->actResetOutputEnergy, &QAction::triggered, [this]() {
		if(checkPassword()) {
			Jn50ViewApp *app = Jn50ViewApp::instance();
			app->rpcConnection()->callShvMethod(app->cliOptions()->converterShvPath() + "/outEnergy", "resetCounter");
		}
	});
	connect(ui->actSettingsConnection, &QAction::triggered, [this]() {
		SettingsDialog dlg(this);
		if(dlg.exec()) {
			QMessageBox::information(this, "JN50 View", tr("The changes will take effect until the application is restarted.")); //Změny se projeví až po restartu aplikace
		}
	});
	connect(ui->actSettingsTresholds, &QAction::triggered, [this]() {
		ThresholdsDialog dlg(this);
		dlg.exec();
	});
	connect(ui->actChangePassword, &QAction::triggered, [this]() {
		if(checkPassword()) {
			bool ok;
			QString pwd = QInputDialog::getText(this, tr("Dialog"), tr("New password:"), QLineEdit::Password, QString(), &ok); //Nové heslo
			if (ok && !pwd.isEmpty()) {
				Settings settings;
				settings.setPassword(pwd);
			}
		}
	});
	connect(ui->actShowLog, &QAction::triggered, [this]() {
		DlgLogView dlg(this);
		Jn50ViewApp *app = Jn50ViewApp::instance();
		dlg.setShvPath(QString::fromStdString(app->cliOptions()->converterShvPath()));
		dlg.exec();
	});
	connect(ui->actHelpAbout, &QAction::triggered, [this]() {
		QMessageBox::about(this
						   , "JN50 View"
						   , "<p><b>JN50 View</b></p>"
							 "<p>ver. " + QCoreApplication::applicationVersion() + "</p>"
							 "<p>Program for visualization of converter JN 50</p>"
							 "<p>2019 Elektroline a.s.</p>"
							 "<p><a href=\"www.elektroline.cz\">www.elektroline.cz</a></p>"
						   );
	});
	connect(ui->actHelpHelp, &QAction::triggered, []() {
		QUrl url = QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/doc/help/html/index.html");
		QDesktopServices::openUrl(url);
	});
	connect(ui->actHelpAppLog, &QAction::triggered, [this]() {
		DlgAppLog dlg(this);
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

bool MainWindow::checkPassword()
{
	bool ok;
	QString pwd = QInputDialog::getText(this, tr("Dialog"), tr("Password:"), QLineEdit::Password, QString(), &ok); //Heslo
	if (ok && !pwd.isEmpty()) {
		Settings settings;
		QString correct_pwd = settings.password();
		if(pwd == correct_pwd) {
			return true;
		}
		else {
			QMessageBox::warning(this, tr("Message"), tr("Bad password!")); //Nesprávné heslo
		}
	}
	return false;
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



