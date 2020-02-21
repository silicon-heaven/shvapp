#include "dlgselectroles.h"
#include "ui_dlgrolesselection.h"

DlgSelectRoles::DlgSelectRoles(QWidget *parent):
	QDialog(parent),
	ui(new Ui::DlgRolesSelection)
{
	ui->setupUi(this);
}

void DlgSelectRoles::init(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path, const std::vector<std::string> &roles)
{
	m_rolesModel = new RolesTreeModel(this);
	m_rolesModel->loadRoles(rpc_connection, acl_etc_roles_node_path);
	ui->tvRoles->setModel(m_rolesModel);
	m_userRoles = roles;

	connect(m_rolesModel, &RolesTreeModel::loadRolesFinished, this, [this](){
		m_rolesModel->setSelectedRoles(m_userRoles);
	});

	connect(m_rolesModel, &RolesTreeModel::loadError, this, [this](QString error){
		ui->lblStatus->setText(error);
	});
}

std::vector<std::string> DlgSelectRoles::selectedRoles()
{
	return m_rolesModel->selectedRoles();
}

void DlgSelectRoles::setUserRoles(const std::vector<std::string> &roles)
{
	m_rolesModel->setSelectedRoles(roles);
}

DlgSelectRoles::~DlgSelectRoles()
{
	delete ui;
}
