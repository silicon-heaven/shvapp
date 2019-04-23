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

	explicit DlgAddEditGrants(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_grants_node_path, DlgAddEditGrants::DialogType dt = DialogType::Add);
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
	std::string grantNameShvPath();

	shv::chainpack::RpcValue::Map createParamsMap();
	shv::chainpack::RpcValue::List grants();
	void setGrants(const shv::chainpack::RpcValue::List &grants);
	Ui::DlgAddEditGrants *ui;
	DialogType m_dialogType;
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
	const std::string &m_aclEtcGrantsNodePath;
};

#endif // DLGADDEDITGRANTS_H
