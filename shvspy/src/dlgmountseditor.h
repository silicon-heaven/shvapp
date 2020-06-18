#ifndef DLGMOUNTSSEDITOR_H
#define DLGMOUNTSEDITOR_H

#include <QDialog>

#include "shv/chainpack/rpcvalue.h"

#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/rpc/clientconnection.h>

namespace Ui {
class DlgMountsEditor;
}

class DlgMountsEditor : public QDialog
{
	Q_OBJECT

public:
	explicit DlgMountsEditor(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection);
	~DlgMountsEditor();
	void init(const std::string &acl_node_path);

private:
	std::string aclEtcMountsNodePath();
	//std::string aclEtcAccessNodePath();

	QString selectedMount();
	void listMounts();
	//void callDeleteAccessForMount(const std::string &mount);

	void onAddMountClicked();
	void onDeleteMountClicked();
	void onEditMountClicked();
	void onTableMountDoubleClicked(QModelIndex ix);

	void setStatusText(const QString &txt);
private:
	Ui::DlgMountsEditor *ui;
	std::string m_aclEtcNodePath;
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
};

#endif // DLGMOUNTSEDITOR_H
