#include "dlgrolesselection.h"
#include "ui_dlgrolesselection.h"

#include <QTimer>

DlgRolesSelection::DlgRolesSelection(QWidget *parent):
	QDialog(parent),
	ui(new Ui::DlgRolesSelection)
{
	ui->setupUi(this);
}

void DlgRolesSelection::init(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path, const shv::chainpack::RpcValue::List &roles)
{
	m_rolesModel = new RolesTreeModel(this);
	m_rolesModel->loadRolesTree(rpc_connection, acl_etc_roles_node_path);
	ui->tvRoles->setModel(m_rolesModel);
	m_userRoles = roles;

	connect(m_rolesModel, &RolesTreeModel::treeLoaded, this, [this](){
		m_rolesModel->setCheckedRoles(m_userRoles);
	});
}

shv::chainpack::RpcValue::List DlgRolesSelection::checkedRoles()
{
	return m_rolesModel->checkedRoles();
}

void DlgRolesSelection::setUserRoles(const shv::chainpack::RpcValue::List &roles)
{
	m_rolesModel->setCheckedRoles(roles);
}

DlgRolesSelection::~DlgRolesSelection()
{
	delete ui;
}
