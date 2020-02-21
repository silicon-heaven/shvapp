#include "dlgrolesselection.h"
#include "ui_dlgrolesselection.h"

DlgRolesSelection::DlgRolesSelection(QWidget *parent):
	QDialog(parent),
	ui(new Ui::DlgRolesSelection)
{
	ui->setupUi(this);
}

void DlgRolesSelection::init(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path, const shv::chainpack::RpcValue::List &roles)
{
	m_rolesModel = new RolesTreeModel(this);
	m_rolesModel->loadRoles(rpc_connection, acl_etc_roles_node_path);
	ui->tvRoles->setModel(m_rolesModel);
	m_userRoles = roles;

	connect(m_rolesModel, &RolesTreeModel::rolesLoaded, this, [this](){
		m_rolesModel->setSelectedRoles(m_userRoles);
	});

	connect(m_rolesModel, &RolesTreeModel::loadError, this, [this](QString error){
		ui->lblStatus->setText(error);
	});
}

shv::chainpack::RpcValue::List DlgRolesSelection::selectedRoles()
{
	return m_rolesModel->secetedRoles();
}

void DlgRolesSelection::setUserRoles(const shv::chainpack::RpcValue::List &roles)
{
	m_rolesModel->setSelectedRoles(roles);
}

DlgRolesSelection::~DlgRolesSelection()
{
	delete ui;
}
