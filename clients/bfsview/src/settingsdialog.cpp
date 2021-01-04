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
		ui->bfsShvPath->setText(settings.bfsShvPath());
		ui->shvBrokerHost->setText(settings.shvBrokerHost());
		ui->shvBrokerPort->setValue(settings.shvBrokerPort());

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
		settings.setBfsShvPath(ui->bfsShvPath->text());
		settings.setShvBrokerUser(ui->shvBrokerUser->text());
		settings.setShvBrokerHost(ui->shvBrokerHost->text());
		settings.setShvBrokerPort(ui->shvBrokerPort->value());
		settings.setPowerFileName(ui->edPowerSwitchesFile->text());
		settings.setCheckPowerFileInterval(ui->edCheckInterval->value());
	}
	Super::done(status);
}
