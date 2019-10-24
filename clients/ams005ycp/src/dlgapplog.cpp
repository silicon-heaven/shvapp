#include "dlgapplog.h"
#include "ui_dlgapplog.h"
#include "ams005ycpapp.h"

#include <QSettings>
#include <QTextStream>

DlgAppLog::DlgAppLog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DlgAppLog)
{
	ui->setupUi(this);

	const QString log_file = QString::fromStdString(Ams005YcpApp::instance()->logFilePath());
	QFile f(log_file);
	if(f.open(QFile::ReadOnly)) {
		QTextStream ts(&f);
		ts.setCodec("utf8");
		QString txt = ts.readAll();
		ui->txtLog->setPlainText(txt);
	}
	{
		QSettings settings;
		QByteArray ba = settings.value("ui/DlgAppLog/geometry").toByteArray();
		restoreGeometry(ba);
	}
}

DlgAppLog::~DlgAppLog()
{
	QSettings settings;
	QByteArray ba = saveGeometry();
	settings.setValue("ui/DlgAppLog/geometry", ba);
	delete ui;
}
