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
	enum DialogType {DtAdd = 0, DtEdit, DtCount};

	explicit DlgAddEditGrants(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_users_node_path, DlgAddEditGrants::DialogType dt = DialogType::DtAdd);
	~DlgAddEditGrants() override;

	DialogType dialogType();
	QString grantName();
	void setGrantName(const QString &grant_name);

	void accept() Q_DECL_OVERRIDE;

private:
	void callAddGrant();
	void callSeGrants();
	void callGetGrants();
	void callCommitChanges();
	std::string grantsShvPath();

	shv::chainpack::RpcValue::List grants();
	void setGrants(const shv::chainpack::RpcValue::List &grants);
	Ui::DlgAddEditGrants *ui;
	DialogType m_dialogType;
	int m_requestedRpcCallsCount = 0;
	shv::iotqt::rpc::ClientConnection *m_rpcConection = nullptr;
	const std::string &m_aclEtcGrantsNodePath;
};

#endif // DLGADDEDITGRANTS_H
