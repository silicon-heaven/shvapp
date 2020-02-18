#ifndef DLGADDEDITROLE_H
#define DLGADDEDITROLE_H

#include <QDialog>

#include "pathsmodel/pathsmodel.h"
#include "pathsmodel/pathstableitemdelegate.h"

#include "shv/chainpack/rpcvalue.h"

#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/rpc/clientconnection.h>

namespace Ui {
class DlgAddEditRole;
}

class DlgAddEditRole : public QDialog
{
	Q_OBJECT

public:
	enum class DialogType {Add = 0, Edit, Count};


	explicit DlgAddEditRole(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_node_path, DlgAddEditRole::DialogType dt = DialogType::Add);
	~DlgAddEditRole() override;

	DialogType dialogType();
	void init(const QString &role_name);
	void accept() Q_DECL_OVERRIDE;

private:

	void callAddRole();
	void callEditRole();
	void callGetRoleSettings();

	void callSetPathsForRole();
	void callGetPathsForRole();

	shv::chainpack::RpcValue::Map paths();

	shv::chainpack::RpcValue::List roles();
	void setRoles(const shv::chainpack::RpcValue::List &roles);

	QString roleName();
	int weight();
	void setWeight(int weight);

	std::string aclEtcRoleNodePath();
	std::string aclEtcPathsNodePath();
	std::string roleShvPath();
	std::string pathShvPath();

	void onAddRowClicked();
	void onDeleteRowClicked();

	Ui::DlgAddEditRole *ui;
	DialogType m_dialogType;
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
	std::string m_aclEtcNodePath;
	PathsModel m_pathsModel;
};

#endif // DLGADDEDITROLE_H
