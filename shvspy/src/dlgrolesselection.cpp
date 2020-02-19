#include "dlgrolesselection.h"
#include "ui_dlgrolesselection.h"

DlgRolesSelection::DlgRolesSelection(QWidget *parent):
	QDialog(parent),
	ui(new Ui::DlgRolesSelection)
{
	ui->setupUi(this);
}

void DlgRolesSelection::init(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path)
{
	m_rolesModel = new RolesTreeModel(this);
	m_rolesModel->loadRolesTree(rpc_connection, acl_etc_roles_node_path);

	ui->tvRoles->setModel(m_rolesModel);
}

DlgRolesSelection::~DlgRolesSelection()
{
	delete ui;
}
