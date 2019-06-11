#ifndef DLGGRANTSEDITOR_H
#define DLGGRANTSEDITOR_H

#include <QDialog>

#include "shv/chainpack/rpcvalue.h"

#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/rpc/clientconnection.h>

namespace Ui {
class DlgGrantsEditor;
}

class DlgGrantsEditor : public QDialog
{
	Q_OBJECT

public:
	explicit DlgGrantsEditor(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection);
	~DlgGrantsEditor();
	void init(const std::string &path);

private:
	std::string aclEtcGrantsNodePath();
	QString selectedGrant();
	void listGrants();

	void onAddGrantClicked();
	void onDelGrantClicked();
	void onEditGrantClicked();
	void onTableGrantDoubleClicked(QModelIndex ix);

	Ui::DlgGrantsEditor *ui;
	std::string m_aclEtcNodePath;
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
};

#endif // DLGGRANTSEDITOR_H
