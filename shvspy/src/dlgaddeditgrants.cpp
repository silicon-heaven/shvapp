#include "dlgaddeditgrants.h"
#include "ui_dlgaddeditgrants.h"

#include "shv/core/log.h"

DlgAddEditGrants::DlgAddEditGrants(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_users_node_path, DlgAddEditGrants::DialogType dt) :
	QDialog(parent),
	ui(new Ui::DlgAddEditGrants),
	m_aclEtcGrantsNodePath(acl_etc_users_node_path)
{
	shvInfo() << m_aclEtcGrantsNodePath;
	ui->setupUi(this);
	m_dialogType = dt;
	bool edit_mode = (m_dialogType == DialogType::DtEdit);

	ui->leGrantName->setEnabled(!edit_mode);
	ui->groupBox->setTitle(edit_mode ? tr("Edit grant") : tr("New grant"));
	setWindowTitle(edit_mode ? tr("Edit grants dialog") : tr("New grant dialog"));

	m_rpcConection = rpc_connection;

	if(m_rpcConection == nullptr){
		ui->lblStatus->setText(tr("Connection to shv does not exist."));
	}
}

DlgAddEditGrants::~DlgAddEditGrants()
{
	delete ui;
}


DlgAddEditGrants::DialogType DlgAddEditGrants::dialogType()
{
return m_dialogType;
}

QString DlgAddEditGrants::grantName()
{
	return ui->leGrantName->text();
}

void DlgAddEditGrants::setGrantName(const QString &grant_name)
{
	ui->leGrantName->setText(grant_name);
	callGetGrants();
}

void DlgAddEditGrants::accept()
{
	if (dialogType() == DtAdd){
		if ((!grantName().isEmpty())){
			ui->lblStatus->setText(tr("Adding new user ..."));
			callAddGrant();
		}
		else {
			ui->lblStatus->setText(tr("User name or password is empty."));
		}
	}
	else if (dialogType() == DtEdit){
		callSeGrants();
	}
}

void DlgAddEditGrants::callAddGrant()
{
	if (m_rpcConection == nullptr)
		return;

	shv::chainpack::RpcValue::Map params;
/*	params["user"] = grant().toStdString();
	params["grants"] = subgrants();

	int rqid = m_rpcConection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(tr("Failed to add user.") + QString::fromStdString(response.error().toString()));
			}
			else{
				QDialog::accept();
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	ui->lblStatus->setText(QString::fromStdString(m_aclEtcUsersNodePath));
	m_rpcConection->callShvMethod(rqid, m_aclEtcUsersNodePath, "addUser", params);*/
}

void DlgAddEditGrants::callSeGrants()
{
	if (m_rpcConection == nullptr)
		return;

	m_requestedRpcCallsCount++;

	int rqid = m_rpcConection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(QString::fromStdString(response.error().toString()));
			}
			else{
				m_requestedRpcCallsCount--;
				callCommitChanges();
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConection->callShvMethod(rqid, grantsShvPath() + "grants", "set", grants());
}

void DlgAddEditGrants::callGetGrants()
{
	if (m_rpcConection == nullptr)
		return;

	ui->lblStatus->setText(tr("Getting settings ..."));

	int rqid = m_rpcConection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(QString::fromStdString(response.error().toString()));
			}
			else{
				(response.result().isList())? setGrants(response.result().toList()) : setGrants(shv::chainpack::RpcValue::List());
				ui->lblStatus->setText("");
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConection->callShvMethod(rqid, grantsShvPath() + "grants", "get");
}

void DlgAddEditGrants::callCommitChanges()
{
	if ((m_requestedRpcCallsCount == 0) && (m_rpcConection != nullptr)){
		int rqid = m_rpcConection->nextRequestId();
		shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConection, rqid, this);

		cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
			if(response.isValid()){
				if(response.isError()) {
					ui->lblStatus->setText(QString::fromStdString(response.error().toString()));
				}
				else{
					QDialog::accept();
				}
			}
			else {
				ui->lblStatus->setText(tr("Request timeout expired"));
			}
		});

		m_rpcConection->callShvMethod(rqid, m_aclEtcGrantsNodePath, "commitChanges");
	}
}

std::string DlgAddEditGrants::grantsShvPath()
{
	return m_aclEtcGrantsNodePath + '/' + grantName().toStdString() + "/";
}

shv::chainpack::RpcValue::List DlgAddEditGrants::grants()
{
	shv::chainpack::RpcValue::List grants;
	QStringList lst = ui->leSubgrants->text().split(",", QString::SplitBehavior::SkipEmptyParts);

	for (int i = 0; i < lst.count(); i++){
		grants.push_back(shv::chainpack::RpcValue::String(lst.at(i).trimmed().toStdString()));
	}

	return grants;
}

void DlgAddEditGrants::setGrants(const shv::chainpack::RpcValue::List &grants)
{
	QString g;

	for (size_t i = 0; i < grants.size(); i++){
		if(i > 0)
			g += ',';
		g += QString::fromStdString(grants[i].toStdString());
	}

	ui->leSubgrants->setText(g);
}
