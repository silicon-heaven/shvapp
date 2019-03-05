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
		ui->predatorShvPath->setText(settings.predatorShvPath());
		ui->shvBrokerHost->setText(settings.shvBrokerHost());
		ui->shvBrokerPort->setValue(settings.shvBrokerPort());
	}
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
		settings.setPredatorShvPath(ui->predatorShvPath->text());
		settings.setShvBrokerHost(ui->shvBrokerHost->text());
		settings.setShvBrokerPort(ui->shvBrokerPort->value());
	}
	Super::done(status);
}
