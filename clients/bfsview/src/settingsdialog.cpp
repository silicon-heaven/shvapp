#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "settings.h"

#include <QFile>
#include <QSettings>

SettingsDialog::SettingsDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::SettingsDialog)
{
	ui->setupUi(this);

	{
		QSettings qsettings;
		Settings settings(qsettings);
		ui->edPowerSwitchesFile->setText(settings.powerFileName());
		//ui->edPowerSwitchName->setText(settings.powerSwitchName());
		ui->edCheckInterval->setValue(settings.powerFileCheckInterval());
	}

	connect(ui->edPowerSwitchesFile, &QLineEdit::textEdited, [this]() {
		bool ok = QFile::exists(ui->edPowerSwitchesFile->text());
		ui->edPowerSwitchesFile->setStyleSheet(ok? QString(): QStringLiteral("background:salmon"));
	});
}

SettingsDialog::~SettingsDialog()
{
	delete ui;
}
/*
QString SettingsDialog::switchName()
{
	return ui->edPowerSwitchName->text();
}

QString SettingsDialog::fileName()
{
	return ui->edPowerSwitchesFile->text();
}

int SettingsDialog::checkInterval()
{
	return ui->edCheckInterval->value();
}
*/
void SettingsDialog::done(int status)
{
	if(status == Accepted) {
		QSettings qsettings;
		Settings settings(qsettings);
		settings.setPowerFileName(ui->edPowerSwitchesFile->text());
		//settings.setPowerSwitchName(ui->edPowerSwitchName->text());
		settings.setCheckPowerFileInterval(ui->edCheckInterval->value());
	}
	Super::done(status);
}
