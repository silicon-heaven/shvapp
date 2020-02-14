#ifndef DLGADDEDITGRANT_H
#define DLGADDEDITGRANT_H

#include <QDialog>

#include "pathsmodel/pathsmodel.h"
#include "pathsmodel/pathstableitemdelegate.h"

#include "shv/chainpack/rpcvalue.h"

#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/rpc/clientconnection.h>

namespace Ui {
class DlgAddEditGrant;
}

class DlgAddEditGrant : public QDialog
{
	Q_OBJECT

public:
	enum class DialogType {Add = 0, Edit, Count};


	explicit DlgAddEditGrant(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_node_path, DlgAddEditGrant::DialogType dt = DialogType::Add);
	~DlgAddEditGrant() override;

	DialogType dialogType();
	void init(const QString &role_name);
	void accept() Q_DECL_OVERRIDE;

private:

	void callAddGrant();
	void callSeGrants();
	void callEditGrant();
	void callGetRoleSettings();

	void callSetGrantPaths();
	void callGetPathsSettings();

	shv::chainpack::RpcValue::Map createParamsMap();
	shv::chainpack::RpcValue::List roles();
	void setRoles(const shv::chainpack::RpcValue::List &roles);

	QString roleName();
	void setWeight(int weight);

	std::string aclEtcRoleNodePath();
	std::string aclEtcPathsNodePath();
	std::string roleShvPath();
	std::string pathShvPath();

	void onAddRowClicked();
	void onDeleteRowClicked();

	Ui::DlgAddEditGrant *ui;
	DialogType m_dialogType;
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
	std::string m_aclEtcNodePath;
	PathsModel m_pathsModel;
};

#endif // DLGADDEDITGRANT_H
