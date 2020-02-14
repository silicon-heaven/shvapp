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
	enum class DialogType {Add = 0, Edit, Count};
	explicit DlgAddEditUser(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_users_node_path, DlgAddEditUser::DialogType dt = DialogType::Add);
	~DlgAddEditUser() override;

	DialogType dialogType();
	std::string user();
	void setUser(const QString &user);
	QString password();

	void accept() Q_DECL_OVERRIDE;

private:
	void onShowPasswordClicked();

	void callSetUserSettings();
	void callGetUserSettings();

	const std::string &aclUsersShvPath();
	std::string userShvPath();

	shv::chainpack::RpcValue::List roles();
	void setRoles(const shv::chainpack::RpcValue::List &roles);

	Ui::DlgAddEditUser *ui;
	DialogType m_dialogType;
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
	const std::string &m_aclEtcUsersNodePath;
	shv::chainpack::RpcValue::Map m_remoteUserSettings;
};

#endif // DLGEDITUSER_H
