#ifndef DLGROLESSELECTION_H
#define DLGROLESSELECTION_H

#include <QDialog>

#include "rolesmodel/rolesmodel.h"

#include <shv/iotqt/rpc/clientconnection.h>

namespace Ui {
class DlgRolesSelection;
}

class DlgRolesSelection : public QDialog
{
	Q_OBJECT

public:
	explicit DlgRolesSelection(QWidget *parent);
	~DlgRolesSelection();

	void init(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path, const shv::chainpack::RpcValue::List &roles);

	shv::chainpack::RpcValue::List selectedRoles();
	void setUserRoles(const shv::chainpack::RpcValue::List &roles);


private:
	shv::chainpack::RpcValue::List m_userRoles;
	RolesTreeModel *m_rolesModel = nullptr;

	Ui::DlgRolesSelection *ui;
};

#endif // DLGROLESSELECTION_H
