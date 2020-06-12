#include "dlgaddeditmount.h"
#include "ui_dlgaddeditmount.h"

#include "shv/core/log.h"

#include <QMessageBox>

static const std::string VALUE_METHOD = "value";
static const std::string SET_VALUE_METHOD = "setValue";

DlgAddEditMount::DlgAddEditMount(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_node_path, DlgAddEditMount::DialogType dt) :
	QDialog(parent),
	ui(new Ui::DlgAddEditMount),
	m_aclEtcNodePath(acl_etc_node_path)
{
	ui->setupUi(this);
	m_dialogType = dt;
	bool edit_mode = (m_dialogType == DialogType::Edit);

	ui->leMountName->setReadOnly(edit_mode);
	ui->groupBox->setTitle(edit_mode ? tr("Edit mount") : tr("New mount"));
	setWindowTitle(edit_mode ? tr("Edit mount dialog") : tr("New mount dialog"));

	m_rpcConnection = rpc_connection;

	setStatusText((m_rpcConnection == nullptr)? tr("Connection to shv does not exist."): QString());
}

DlgAddEditMount::~DlgAddEditMount()
{
	delete ui;
}

DlgAddEditMount::DialogType DlgAddEditMount::dialogType()
{
	return m_dialogType;
}

void DlgAddEditMount::init(const QString &mount_name)
{
	ui->leMountName->setText(mount_name);
	callGetMountSettings();
}

void DlgAddEditMount::accept()
{
	if (dialogType() == DialogType::Add){
		if (!mountName().isEmpty()){
			setStatusText(tr("Adding new mount ..."));
			callAddMount();
		}
		else {
			setStatusText(tr("Mount name is empty."));
		}
	}
	else if (dialogType() == DialogType::Edit){
		callEditMount();
	}
}

void DlgAddEditMount::callAddMount()
{
	if (m_rpcConnection == nullptr)
		return;

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				setStatusText(tr("Failed to add mount.") + QString::fromStdString(response.error().toString()));
			}
		}
		else{
			setStatusText(tr("Request timeout expired"));
		}
	});

	m_mount.mountPoint = ui->leMountPoint->text().toStdString();
	m_mount.description = ui->leDescription->text().toStdString();

	shv::chainpack::RpcValue::List params{mountName().toStdString(), m_mount.toRpcValueMap()};
	m_rpcConnection->callShvMethod(rqid, aclEtcMountNodePath(), SET_VALUE_METHOD, params);
}

void DlgAddEditMount::callEditMount()
{
	if (m_rpcConnection == nullptr)
		return;

	setStatusText(tr("Updating mount ..."));

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				setStatusText(tr("Failed to edit mount.") + QString::fromStdString(response.error().toString()));
			}
		}
		else{
			setStatusText(tr("Request timeout expired"));
		}
	});

	m_mount.mountPoint = ui->leMountPoint->text().toStdString();
	m_mount.description = ui->leDescription->text().toStdString();

	shv::chainpack::RpcValue::List params{mountName().toStdString(), m_mount.toRpcValueMap()};
	m_rpcConnection->callShvMethod(rqid, aclEtcMountNodePath(), SET_VALUE_METHOD, params);
}

void DlgAddEditMount::callGetMountSettings()
{
	if(m_rpcConnection == nullptr){
		return;
	}

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if (response.isError()) {
				setStatusText(tr("Failed to call method ls.") + QString::fromStdString(response.error().toString()));
			}
			else{
				m_mount = shv::broker::AclMountDef::fromRpcValue(response.result());
				ui->leMountPoint->setText(QString::fromStdString(m_mount.mountPoint));
				ui->leDescription->setText(QString::fromStdString(m_mount.description));
			}
		}
		else{
			setStatusText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, mountShvPath(), VALUE_METHOD);
}

QString DlgAddEditMount::mountName()
{
	return ui->leMountName->text();
}

std::string DlgAddEditMount::aclEtcMountNodePath()
{
	return m_aclEtcNodePath + "/mounts";
}

std::string DlgAddEditMount::mountShvPath()
{
	return aclEtcMountNodePath() + '/' + mountName().toStdString();
}

void DlgAddEditMount::setStatusText(const QString &txt)
{
	if(txt.isEmpty()) {
		ui->lblStatus->hide();
	}
	else {
		ui->lblStatus->show();
		ui->lblStatus->setText(txt);
	}
}

