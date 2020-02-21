#ifndef DLGADDEDITROLE_H
#define DLGADDEDITROLE_H

#include <QDialog>

#include "accessmodel/accessmodel.h"

#include "shv/chainpack/rpcvalue.h"

#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/rpc/clientconnection.h>

#include <shv/broker/aclmanager.h>

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

	void callSetAccessForRole();
	void callGetPathsForRole();

	shv::chainpack::RpcValue::Map paths();

	std::vector<std::string> roles();
	void setRoles(const std::vector<std::string> &roles);

	QString roleName();
	int weight();
	void setWeight(int weight);

	std::string aclEtcRoleNodePath();
	std::string aclEtcAcessNodePath();
	std::string roleShvPath();
	std::string accessShvPath();

	void onAddRowClicked();
	void onDeleteRowClicked();

	Ui::DlgAddEditRole *ui;
	DialogType m_dialogType;
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
	std::string m_aclEtcNodePath;
	AccessModel m_accessModel;
	shv::broker::AclRole m_role;
};

#endif // DLGADDEDITROLE_H
