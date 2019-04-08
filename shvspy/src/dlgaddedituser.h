#ifndef DLGADDEDITUSER_H
#define DLGADDEDITUSER_H

#include <QDialog>

#include "shv/chainpack/rpcvalue.h"

#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/rpc/clientconnection.h>

namespace Ui {
class DlgAddEditUser;
}

class DlgAddEditUser : public QDialog
{
	Q_OBJECT

public:
	enum DialogType {DtAddUser = 0, DtEditUser, DtCount};
	explicit DlgAddEditUser(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &usersNodePath, DlgAddEditUser::DialogType dt = DialogType::DtAddUser);
	~DlgAddEditUser() override;

	DialogType dialogType();
	QString user();
	void setUser(const QString &user);
	QString password();

	void accept() Q_DECL_OVERRIDE;

private:
	void onShowPasswordClicked();
	void showPasswordItems(bool visible);

	void callAddUser();
	void callChangePassword();
	void callSetGrants();
	void callGetGrants();
	void callCommitChanges();
	std::string userShvPath();

	shv::chainpack::RpcValue::List grants();
	void setGrants(const shv::chainpack::RpcValue::List &grants);
	Ui::DlgAddEditUser *ui;
	DialogType m_dialogType;
	int m_requestedRpcCallsCount = 0;
	shv::iotqt::rpc::ClientConnection *m_rpcConection = nullptr;
	const std::string &m_aclEtcUsersNodePath;
};

#endif // DLGEDITUSER_H
