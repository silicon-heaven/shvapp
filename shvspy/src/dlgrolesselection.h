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

	void init(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path);
	~DlgRolesSelection();

private:
	RolesTreeModel *m_rolesModel = nullptr;

	Ui::DlgRolesSelection *ui;
};

#endif // DLGROLESSELECTION_H
