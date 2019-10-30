#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ams005ycpapp.h"
#include "appclioptions.h"
#include "settingsdialog.h"
#include "settings.h"

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
	setWindowTitle(tr("Yard Control Panel"));

	setGeometry(0, 0, 1280, 800);

	connect(ui->actHelpAbout, &QAction::triggered, [this]() {
		QMessageBox::about(this
						   , "JN50 View"
						   , "<p><b>Yard Control Panel</b></p>"
							 "<p>ver. " + QCoreApplication::applicationVersion() + "</p>"
							 "<p>Yard control panel and route status visualization</p>"
							 "<p>2019 Elektroline a.s.</p>"
							 "<p><a href=\"www.elektroline.cz\">www.elektroline.cz</a></p>"
						   );
	});
	connect(ui->actHelpHelp, &QAction::triggered, []() {
		QUrl url = QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/doc/help/html/index.html");
		QDesktopServices::openUrl(url);
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



