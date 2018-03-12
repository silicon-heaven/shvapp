#include "dlgsubscriptionparameters.h"
#include "ui_dlgsubscriptionparameters.h"

DlgSubscriptionParameters::DlgSubscriptionParameters(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DlgSubscriptionParameters)
{
	ui->setupUi(this);
}

DlgSubscriptionParameters::~DlgSubscriptionParameters()
{
	delete ui;
}
/*
void DlgSubscriptionParameters::setSubscriptionParameters(const qfopcua::SubscriptionParameters &params)
{
	ui->chkPublishingEnabled->setChecked(params.publishingEnabled);
	ui->edPriority->setValue(params.priority);
	ui->edPublishInterval->setValue(params.requestedPublishingInterval);
	ui->edKeepAliveCount->setValue(params.requestedMaxKeepAliveCount);
	ui->edLifetimeCount->setValue(params.requestedLifetimeCount);
	ui->edMaxNotificationPerPublish->setValue(params.maxNotificationsPerPublish);
}

qfopcua::SubscriptionParameters DlgSubscriptionParameters::subscriptionParameters()
{
	qfopcua::SubscriptionParameters params;
	params.publishingEnabled = ui->chkPublishingEnabled->isChecked();
	params.priority = ui->edPriority->value();
	params.requestedPublishingInterval = ui->edPublishInterval->value();
	params.requestedMaxKeepAliveCount = ui->edKeepAliveCount->value();
	params.requestedLifetimeCount = ui->edLifetimeCount->value();
	params.maxNotificationsPerPublish = ui->edMaxNotificationPerPublish->value();
	return params;
}
*/
