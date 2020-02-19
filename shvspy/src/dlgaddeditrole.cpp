#include "dlgaddeditrole.h"
#include "ui_dlgaddeditrole.h"

#include "shv/core/log.h"

static const std::string WEIGHT = "weight";
static const std::string ROLES = "roles";

static const std::string VALUE_METHOD = "value";
static const std::string SET_VALUE_METHOD = "setValue";


DlgAddEditRole::DlgAddEditRole(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_node_path, DlgAddEditRole::DialogType dt) :
	QDialog(parent),
	ui(new Ui::DlgAddEditRole),
	m_aclEtcNodePath(acl_etc_node_path)
{
	ui->setupUi(this);
	m_dialogType = dt;
	bool edit_mode = (m_dialogType == DialogType::Edit);

	ui->leRoleName->setReadOnly(edit_mode);
	ui->groupBox->setTitle(edit_mode ? tr("Edit role") : tr("New role"));
	setWindowTitle(edit_mode ? tr("Edit role dialog") : tr("New role dialog"));

	ui->tvPaths->setModel(&m_accessModel);
	ui->tvPaths->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	ui->tvPaths->verticalHeader()->setDefaultSectionSize(static_cast<int>(fontMetrics().height() * 1.3));
	ui->tvPaths->setColumnWidth(AccessModel::Columns::ColPath, frameGeometry().width() * 0.6);

	connect(ui->tbAddRow, &QToolButton::clicked, this, &DlgAddEditRole::onAddRowClicked);
	connect(ui->tbDeleteRow, &QToolButton::clicked, this, &DlgAddEditRole::onDeleteRowClicked);

	m_rpcConnection = rpc_connection;

	if(m_rpcConnection == nullptr){
		ui->lblStatus->setText(tr("Connection to shv does not exist."));
	}
}

DlgAddEditRole::~DlgAddEditRole()
{
	delete ui;
}

DlgAddEditRole::DialogType DlgAddEditRole::dialogType()
{
	return m_dialogType;
}

void DlgAddEditRole::init(const QString &role_name)
{
	ui->leRoleName->setText(role_name);
	callGetRoleSettings();
	callGetPathsForRole();
}

void DlgAddEditRole::accept()
{
	if (dialogType() == DialogType::Add){
		if (!roleName().isEmpty()){
			ui->lblStatus->setText(tr("Adding new role ..."));
			callAddRole();
		}
		else {
			ui->lblStatus->setText(tr("Role name is empty."));
		}
	}
	else if (dialogType() == DialogType::Edit){
		callEditRole();
	}
}

void DlgAddEditRole::callAddRole()
{
	if (m_rpcConnection == nullptr)
		return;

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(tr("Failed to add role.") + QString::fromStdString(response.error().toString()));
			}
			else{
				callSetAccessForRole();
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	shv::chainpack::RpcValue::Map role_settings;
	role_settings[ROLES] = roles();
	role_settings[WEIGHT] = weight();

	shv::chainpack::RpcValue::List params{roleName().toStdString(), role_settings};
	m_rpcConnection->callShvMethod(rqid, aclEtcRoleNodePath(), SET_VALUE_METHOD, params);
}

void DlgAddEditRole::callEditRole()
{
	if (m_rpcConnection == nullptr)
		return;

	ui->lblStatus->setText("Updating role");

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(tr("Failed to edit role.") + QString::fromStdString(response.error().toString()));
			}
			else{
				callSetAccessForRole();
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	shv::chainpack::RpcValue::Map role_settings;
	role_settings[ROLES] = roles();
	role_settings[WEIGHT] = weight();

	shv::chainpack::RpcValue::List params{roleName().toStdString(), role_settings};
	m_rpcConnection->callShvMethod(rqid, aclEtcRoleNodePath(), SET_VALUE_METHOD, params);
}

void DlgAddEditRole::callGetRoleSettings()
{
	if(m_rpcConnection == nullptr){
		return;
	}

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if (response.isError()) {
				ui->lblStatus->setText(tr("Failed to call method ls.") + QString::fromStdString(response.error().toString()));
			}
			else{
				if (response.result().isMap()){
					shv::chainpack::RpcValue::Map res = response.result().toMap();
					setRoles(res.value(ROLES).toList());
					setWeight((res.value(WEIGHT).isValid() && res.value(WEIGHT).isInt()) ? res.value(WEIGHT).toInt() : 0);
				}
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, roleShvPath(), VALUE_METHOD);
}

void DlgAddEditRole::callGetPathsForRole()
{
	if (m_rpcConnection == nullptr)
		return;

	ui->lblStatus->setText(tr("Getting paths ..."));

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(QString::fromStdString(response.error().toString()));
			}
			else{
				(response.result().isMap())? m_accessModel.setPaths(response.result().toMap()) : m_accessModel.setPaths(shv::chainpack::RpcValue::Map());
				ui->lblStatus->setText("");
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, accessShvPath(), VALUE_METHOD);
}

shv::chainpack::RpcValue::Map DlgAddEditRole::paths()
{
	return m_accessModel.paths();
}

void DlgAddEditRole::callSetAccessForRole()
{
	if (m_rpcConnection == nullptr)
		return;

	ui->lblStatus->setText(tr("Updating paths ..."));

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
			if(response.isError()) {
				ui->lblStatus->setText(QString::fromStdString(response.error().toString()));
			}
			else{
				QDialog::accept();
			}
		}
		else{
			ui->lblStatus->setText(tr("Request timeout expired"));
		}
	});


	shv::chainpack::RpcValue::List params{roleName().toStdString(), paths()};
	m_rpcConnection->callShvMethod(rqid, aclEtcAcessNodePath(), SET_VALUE_METHOD, params);
}

shv::chainpack::RpcValue::List DlgAddEditRole::roles()
{
	shv::chainpack::RpcValue::List roles;
	QStringList lst = ui->leRoles->text().split(",", QString::SplitBehavior::SkipEmptyParts);

	for (int i = 0; i < lst.count(); i++){
		roles.push_back(lst.at(i).trimmed().toStdString());
	}

	return roles;
}

void DlgAddEditRole::setRoles(const shv::chainpack::RpcValue::List &roles)
{
	QString g;

	for (size_t i = 0; i < roles.size(); i++){
		if(i > 0)
			g += ',';
		g += QString::fromStdString(roles[i].toStdString());
	}

	ui->leRoles->setText(g);
}

QString DlgAddEditRole::roleName()
{
	return ui->leRoleName->text();
}

int DlgAddEditRole::weight()
{
	return ui->sbWeight->value();
}

void DlgAddEditRole::setWeight(int weight)
{
	ui->sbWeight->setValue(weight);
}

std::string DlgAddEditRole::aclEtcRoleNodePath()
{
	return m_aclEtcNodePath + "roles";
}

std::string DlgAddEditRole::aclEtcAcessNodePath()
{
	return m_aclEtcNodePath + "access";
}

std::string DlgAddEditRole::roleShvPath()
{
	return aclEtcRoleNodePath() + '/' + roleName().toStdString() + "/";
}

std::string DlgAddEditRole::accessShvPath()
{
	return aclEtcAcessNodePath() + '/' + roleName().toStdString() + "/";
}

void DlgAddEditRole::onAddRowClicked()
{
	m_accessModel.addPath();
}

void DlgAddEditRole::onDeleteRowClicked()
{
	m_accessModel.deletePath(ui->tvPaths->currentIndex().row());
}
