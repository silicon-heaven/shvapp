#include "dlgselectroles.h"
#include "ui_dlgselectroles.h"

DlgSelectRoles::DlgSelectRoles(QWidget *parent):
	QDialog(parent),
	ui(new Ui::DlgSelectRoles)
{
	ui->setupUi(this);
}

void DlgSelectRoles::init(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path, const std::vector<std::string> &roles)
{
	m_rolesTreeModel = new RolesTreeModel(this);
	m_rolesTreeModel->load(rpc_connection, acl_etc_roles_node_path);
	ui->tvRoles->setModel(m_rolesTreeModel);
	m_userRoles = roles;

	connect(m_rolesTreeModel, &RolesTreeModel::loadFinished, this, [this](){
		m_rolesTreeModel->setSelectedRoles(m_userRoles);
	});

	connect(m_rolesTreeModel, &RolesTreeModel::loadError, this, [this](QString error){
		ui->lblStatus->setText(error);
	});
}

std::vector<std::string> DlgSelectRoles::selectedRoles()
{
	return m_rolesTreeModel->selectedRoles();
}

void DlgSelectRoles::setUserRoles(const std::vector<std::string> &roles)
{
	m_rolesTreeModel->setSelectedRoles(roles);
}

DlgSelectRoles::~DlgSelectRoles()
{
	delete ui;
}
