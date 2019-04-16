#include "dlgaddeditgrants.h"
#include "ui_dlgaddeditgrants.h"

#include "shv/core/log.h"

static const std::string WEIGHT = "weight";
static const std::string GRANTS = "grants";

DlgAddEditGrants::DlgAddEditGrants(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_grants_node_path, DlgAddEditGrants::DialogType dt) :
	QDialog(parent),
	ui(new Ui::DlgAddEditGrants),
	m_aclEtcGrantsNodePath(acl_etc_grants_node_path)
{
	ui->setupUi(this);
	m_dialogType = dt;
	bool edit_mode = (m_dialogType == DialogType::Edit);

	ui->leGrantName->setEnabled(!edit_mode);
	ui->groupBox->setTitle(edit_mode ? tr("Edit grant") : tr("New grant"));
	setWindowTitle(edit_mode ? tr("Edit grant dialog") : tr("New grant dialog"));

	m_rpcConnection = rpc_connection;

	if(m_rpcConnection == nullptr){
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

void DlgAddEditGrants::init(const QString &grant_name)
{
	if(m_rpcConnection == nullptr){
		return;
	}

	ui->leGrantName->setText(grant_name);

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if (response.isError()) {
				ui->lblStatus->setText(tr("Failed to call method ls.") + QString::fromStdString(response.error().toString()));
			}
			else{
				if (response.result().isList()){
					shv::chainpack::RpcValue::List nodes = response.result().toList();

					for (size_t i = 0; i < nodes.size(); i++){
						if (nodes.at(i).toStdString() == GRANTS){
							callGetGrants();
						}
						if (nodes.at(i).toStdString() == WEIGHT){
							callGetWeight();
						}
					}
				}
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, grantNameShvPath(), "ls");
}

QString DlgAddEditGrants::grantName()
{
	return ui->leGrantName->text();
}

void DlgAddEditGrants::accept()
{
	if (dialogType() == DialogType::Add){
		if ((!grantName().isEmpty())){
			ui->lblStatus->setText(tr("Adding new grant ..."));
			callAddGrant();
		}
		else {
			ui->lblStatus->setText(tr("Grant name or grants is empty."));
		}
	}
	else if (dialogType() == DialogType::Edit){
		callEditGrant();
	}
}

void DlgAddEditGrants::callAddGrant()
{
	if (m_rpcConnection == nullptr)
		return;

	shv::chainpack::RpcValue::Map params = createParamsMap();

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(tr("Failed to add grant.") + QString::fromStdString(response.error().toString()));
			}
			else{
				QDialog::accept();
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	ui->lblStatus->setText(QString::fromStdString(m_aclEtcGrantsNodePath));
	m_rpcConnection->callShvMethod(rqid, m_aclEtcGrantsNodePath, "addGrant", params);
}

void DlgAddEditGrants::callGetGrants()
{
	if (m_rpcConnection == nullptr)
		return;

	ui->lblStatus->setText(tr("Loading grants ..."));

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

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

	m_rpcConnection->callShvMethod(rqid, grantNameShvPath() + GRANTS, "get");
}

void DlgAddEditGrants::callGetWeight()
{
	if (m_rpcConnection == nullptr)
		return;

	ui->lblStatus->setText(tr("Loading weight ..."));

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(QString::fromStdString(response.error().toString()));
			}
			else{
				ui->sbWeight->setValue((response.result().isInt())? response.result().toInt() : -1);
				ui->lblStatus->setText("");
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, grantNameShvPath() + WEIGHT, "get");
}

void DlgAddEditGrants::callEditGrant()
{
	if (m_rpcConnection == nullptr)
		return;

	shv::chainpack::RpcValue::Map params = createParamsMap();

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(tr("Failed to edit grant.") + QString::fromStdString(response.error().toString()));
			}
			else{
				QDialog::accept();
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	ui->lblStatus->setText(QString::fromStdString(m_aclEtcGrantsNodePath));
	m_rpcConnection->callShvMethod(rqid, m_aclEtcGrantsNodePath, "editGrant", params);
}

std::string DlgAddEditGrants::grantNameShvPath()
{
	return m_aclEtcGrantsNodePath + '/' + grantName().toStdString() + "/";
}

shv::chainpack::RpcValue::Map DlgAddEditGrants::createParamsMap()
{
	shv::chainpack::RpcValue::Map params;
	params["grantName"] = grantName().toStdString();

	shv::chainpack::RpcValue::List g = grants();
	if(!g.empty()){
		params[GRANTS] = g;
	}

	int weight = ui->sbWeight->value();
	if (weight > -1){
		params[WEIGHT] = weight;
	}
	return params;
}

shv::chainpack::RpcValue::List DlgAddEditGrants::grants()
{
	shv::chainpack::RpcValue::List grants;
	QStringList lst = ui->leGrants->text().split(",", QString::SplitBehavior::SkipEmptyParts);

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

	ui->leGrants->setText(g);
}
