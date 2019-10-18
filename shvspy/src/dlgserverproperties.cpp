#include "dlgserverproperties.h"
#include "ui_dlgserverproperties.h"

#include <shv/chainpack/irpcconnection.h>

#include <QSettings>

namespace cp = shv::chainpack;

DlgServerProperties::DlgServerProperties(QWidget *parent) :
	Super(parent),
	ui(new Ui::DlgServerProperties)
{
	ui->setupUi(this);

	ui->cbxConnectionType->addItem(tr("Client"), "client");
	ui->cbxConnectionType->addItem(tr("Device"), "device");
	connect(ui->cbxConnectionType, QOverload<int>::of(&QComboBox::currentIndexChanged), ui->grpDevice, [this](int ix) {
		ui->grpDevice->setEnabled(ix == 1);
	});

	ui->cbxConnectionType->setCurrentIndex(0);

	ui->rpc_protocolType->addItem(cp::Rpc::protocolTypeToString(cp::Rpc::ProtocolType::ChainPack), (int)cp::Rpc::ProtocolType::ChainPack);
	ui->rpc_protocolType->addItem(cp::Rpc::protocolTypeToString(cp::Rpc::ProtocolType::Cpon), (int)cp::Rpc::ProtocolType::Cpon);
	ui->rpc_protocolType->addItem(cp::Rpc::protocolTypeToString(cp::Rpc::ProtocolType::JsonRpc), (int)cp::Rpc::ProtocolType::JsonRpc);
	ui->rpc_protocolType->setCurrentIndex(0);

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
	ret["connectionType"] = ui->cbxConnectionType->currentData().toString();
	ret["rpc.protocolType"] = ui->rpc_protocolType->currentData().toInt();
	ret["rpc.reconnectInterval"] = ui->rpc_reconnectInterval->value();
	ret["rpc.heartbeatInterval"] = ui->rpc_heartbeatInterval->value();
	ret["rpc.defaultRpcTimeout"] = ui->rpc_timeout->value();
	ret["device.id"] = ui->device_id->text().trimmed();
	ret["device.mountPoint"] = ui->device_mountPoint->text().trimmed();
	ret["subscriptions"] = m_subscriptions;
	ret["muteHeartBeats"] = ui->chkMuteHeartBeats->isChecked();
	return ret;
}

void DlgServerProperties::setServerProperties(const QVariantMap &props)
{
	m_subscriptions = props.value(QStringLiteral("subscriptions")).toList();

	ui->edName->setText(props.value("name").toString());
	ui->edHost->setText(props.value("host").toString());
	ui->edPort->setValue(props.value("port", shv::chainpack::IRpcConnection::DEFAULT_RPC_BROKER_PORT).toInt());
	ui->edUser->setText(props.value("user").toString());
	ui->edPassword->setText(props.value("password").toString());
	{
		QVariant v = props.value("rpc.reconnectInterval");
		if(v.isValid())
			ui->rpc_reconnectInterval->setValue(v.toInt());
	}
	{
		QVariant v = props.value("rpc.heartbeatInterval");
		if(v.isValid())
			ui->rpc_heartbeatInterval->setValue(v.toInt());
	}
	{
		QVariant v = props.value("rpc.defaultRpcTimeout");
		if(v.isValid())
			ui->rpc_timeout->setValue(v.toInt());
	}
	{
		QVariant v = props.value("device.id");
		if(v.isValid())
			ui->device_id->setText(v.toString());
	}
	{
		QVariant v = props.value("device.mountPoint");
		if(v.isValid())
			ui->device_mountPoint->setText(v.toString());
	}

	QString conn_type = props.value("connectionType").toString();
	ui->cbxConnectionType->setCurrentIndex(0);
	for (int i = 0; i < ui->cbxConnectionType->count(); ++i) {
		if(ui->cbxConnectionType->itemData(i).toString() == conn_type) {
			ui->cbxConnectionType->setCurrentIndex(i);
			break;
		}
	}

	int proto_type = props.value("rpc.protocolType").toInt();
	ui->rpc_protocolType->setCurrentIndex(0);
	for (int i = 0; i < ui->rpc_protocolType->count(); ++i) {
		if(ui->rpc_protocolType->itemData(i).toInt() == proto_type) {
			ui->rpc_protocolType->setCurrentIndex(i);
			break;
		}
	}
	ui->chkMuteHeartBeats->setChecked(props.value("muteHeartBeats").toBool());
}

void DlgServerProperties::done(int res)
{
	QSettings settings;
	settings.setValue(QStringLiteral("ui/dlgServerProperties/geometry"), saveGeometry());
	Super::done(res);
}
