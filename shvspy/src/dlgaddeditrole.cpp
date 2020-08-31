#include "dlgaddeditrole.h"
#include "ui_dlgaddeditrole.h"

#include "shv/core/log.h"

#include <QMessageBox>

void AccessPathsView::commitData(QWidget *editor)
{
	try {
		Super::commitData(editor);
	}
	catch (const std::exception &e) {
		QMessageBox::warning(this, tr("Commit data error"), e.what());
	}
}

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

	setStatusText((m_rpcConnection == nullptr)? tr("Connection to shv does not exist."): QString());
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
	if (!m_accessModel.isRulesValid()){
		setStatusText(tr("Invalid paths."));
		return;
	}

	if (dialogType() == DialogType::Add){
		setStatusText(tr("Adding new role ..."));
		callSetRoleSettings();
	}
	else if (dialogType() == DialogType::Edit){
		setStatusText(tr("Updating role ..."));
		callSetRoleSettings();
	}
}

void DlgAddEditRole::callSetRoleSettings()
{
	if (m_rpcConnection == nullptr)
		return;

	if (roleName().isEmpty()){
		setStatusText(tr("Error: role name is empty."));
		return;
	}
	else if (!m_accessModel.rules().isValid()){
		setStatusText(tr("Error: paths must be a valid Cpon."));
		return;
	}

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if(response.isError()) {
				setStatusText(tr("Failed:") + QString::fromStdString(response.error().toString()));
			}
			else{
				callSetPathsForRole();
			}
		}
		else{
			setStatusText(tr("Request timeout expired"));
		}
	});

	m_role.roles = roles();
	m_role.weight = weight();

	shv::chainpack::RpcValue::List params{roleName().toStdString(), m_role.toRpcValueMap()};
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
				setStatusText(tr("Failed to call method ls.") + QString::fromStdString(response.error().toString()));
			}
			else{
				m_role = shv::broker::AclRole::fromRpcValue(response.result());
				setRoles(m_role.roles);
				setWeight(m_role.weight);
			}
		}
		else{
			setStatusText(tr("Request timeout expired"));
		}
	});

	m_rpcConnection->callShvMethod(rqid, roleShvPath(), VALUE_METHOD);
}

void DlgAddEditRole::callGetPathsForRole()
{
	if (m_rpcConnection == nullptr)
		return;

	setStatusText(tr("Getting paths ..."));

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
			if(response.isError()) {
				setStatusText(tr("Get paths error: %1").arg(QString::fromStdString(response.error().toString())));
			}
			else{
				m_accessModel.setRules(response.result());
				setStatusText(QString());
				return;
			}
		}
		else{
			setStatusText(tr("Request timeout expired"));
		}
		m_accessModel.setRules(shv::chainpack::RpcValue());
	});

	m_rpcConnection->callShvMethod(rqid, accessShvPath(), VALUE_METHOD);
}

void DlgAddEditRole::callSetPathsForRole()
{
	if (m_rpcConnection == nullptr) {
		return;
	}

	setStatusText(tr("Updating paths ..."));

	int rqid = m_rpcConnection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(m_rpcConnection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
			if(response.isError()) {
				setStatusText(tr("Update paths error: %1").arg(QString::fromStdString(response.error().toString())));
			}
			else{
				QDialog::accept();
			}
		}
		else{
			setStatusText(tr("Request timeout expired"));
		}
	});

	shv::chainpack::RpcValue::List params{roleName().toStdString(), paths()};
	m_rpcConnection->callShvMethod(rqid, aclEtcAcessNodePath(), SET_VALUE_METHOD, params);
}

shv::chainpack::RpcValue DlgAddEditRole::paths()
{
	return m_accessModel.rules();
}

std::vector<std::string> DlgAddEditRole::roles()
{
	std::vector<std::string> roles;
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
		auto skip_empty_parts = QString::SkipEmptyParts;
#else
		auto skip_empty_parts = Qt::SkipEmptyParts;
#endif
	QStringList lst = ui->leRoles->text().split(",", skip_empty_parts);

	for (int i = 0; i < lst.count(); i++){
		roles.push_back(lst.at(i).trimmed().toStdString());
	}

	return roles;
}

void DlgAddEditRole::setRoles(const std::vector<std::string> &roles)
{
	QString g;

	for (size_t i = 0; i < roles.size(); i++){
		if(i > 0)
			g += ',';
		g += QString::fromStdString(roles[i]);
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
	return m_aclEtcNodePath + "/roles";
}

std::string DlgAddEditRole::aclEtcAcessNodePath()
{
	return m_aclEtcNodePath + "/access";
}

std::string DlgAddEditRole::roleShvPath()
{
	return aclEtcRoleNodePath() + '/' + roleName().toStdString();
}

std::string DlgAddEditRole::accessShvPath()
{
	return aclEtcAcessNodePath() + '/' + roleName().toStdString();
}

void DlgAddEditRole::onAddRowClicked()
{
	m_accessModel.addRule();
}

void DlgAddEditRole::onDeleteRowClicked()
{
	m_accessModel.deleteRule(ui->tvPaths->currentIndex().row());
}

void DlgAddEditRole::setStatusText(const QString &txt)
{
	if(txt.isEmpty()) {
		ui->lblStatus->hide();
	}
	else {
		ui->lblStatus->show();
		ui->lblStatus->setText(txt);
	}
}

