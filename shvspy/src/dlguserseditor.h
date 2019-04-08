#ifndef DLGUSERSEDITOR_H
#define DLGUSERSEDITOR_H

#include <QDialog>

#include <shv/iotqt/rpc/rpcresponsecallback.h>

namespace Ui {
class DlgUsersEditor;
}

class DlgUsersEditor : public QDialog
{
	Q_OBJECT

public:
	explicit DlgUsersEditor(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection);
	~DlgUsersEditor();
	void init(const std::string &path);

private:
	Ui::DlgUsersEditor *ui;

	void listUsers();
	QString selectedUser();

	void onAddUserClicked();
	void onDelUserClicked();
	void onEditUserClicked();
	void onTableUsersDoubleClicked(QModelIndex ix);

	shv::iotqt::rpc::ClientConnection *m_rpcConection = nullptr;
	std::string m_aclEtcUsersNodePath;
};

#endif // DLGUSERSEDITOR_H
