#include "dlgserverproperties.h"
#include "ui_dlgserverproperties.h"

//#include "theapp.h"

#include <QSettings>
#include <QDebug>

DlgServerProperties::DlgServerProperties(QWidget *parent) :
	Super(parent),
	ui(new Ui::DlgServerProperties)
{
	ui->setupUi(this);

	QSettings settings;
	restoreGeometry(settings.value(QStringLiteral("ui/dlgServerProperties/geometry")).toByteArray());
}

DlgServerProperties::~DlgServerProperties()
{
	delete ui;
}

QVariantMap DlgServerProperties::serverProperties() const
{
	QVariantMap ret;
	ret["name"] = ui->edName->text();
	ret["host"] = ui->edHost->text();
	ret["port"] = ui->edPort->value();
	ret["user"] = ui->edUser->text();
	ret["password"] = ui->edPassword->text();
	return ret;
}

void DlgServerProperties::setServerProperties(const QVariantMap &props)
{
	ui->edName->setText(props.value("name").toString());
	ui->edHost->setText(props.value("host").toString());
	ui->edPort->setValue(props.value("port", 4840).toInt());
	ui->edUser->setText(props.value("user").toString());
	ui->edPassword->setText(props.value("password").toString());
}

void DlgServerProperties::done(int res)
{
	qDebug() << Q_FUNC_INFO;
	QSettings settings;
	settings.setValue(QStringLiteral("ui/dlgServerProperties/geometry"), saveGeometry());
	Super::done(res);
}
