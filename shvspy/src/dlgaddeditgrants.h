#ifndef DLGADDEDITGRANTS_H
#define DLGADDEDITGRANTS_H

#include <QDialog>

#include "shv/chainpack/rpcvalue.h"

#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/rpc/clientconnection.h>

namespace Ui {
class DlgAddEditGrants;
}

class DlgAddEditGrants : public QDialog
{
	Q_OBJECT

public:
	enum class DialogType {Add = 0, Edit, Count};
	enum Columns {ColPath, ColGrant, ColWeight};

	explicit DlgAddEditGrants(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_node_path, DlgAddEditGrants::DialogType dt = DialogType::Add);
	~DlgAddEditGrants() override;

	DialogType dialogType();
	void init(const QString &grant_name);
	void accept() Q_DECL_OVERRIDE;

private:
	QString grantName();

	void callAddGrant();
	void callSeGrants();
	void callGetGrants();
	void callSetWeight();
	void callGetWeight();
	void callEditGrant();
	void callGetGrantInfo();
	std::string grantNameShvPath();

	void callSetGrantPaths();
	void callGetGrantPaths();

	void setPaths(const shv::chainpack::RpcValue::Map &paths);
	shv::chainpack::RpcValue::Map paths();

	std::string aclEtcGrantsNodePath();
	std::string aclEtcPathsNodePath();

	shv::chainpack::RpcValue::Map createParamsMap();
	shv::chainpack::RpcValue::List grants();
	void setGrants(const shv::chainpack::RpcValue::List &grants);

	void onAddRowClicked();
	void onDeleteRowClicked();

	Ui::DlgAddEditGrants *ui;
	DialogType m_dialogType;
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
	std::string m_aclEtcNodePath;
};

#endif // DLGADDEDITGRANTS_H
