#ifndef DLGPATHSEDITOR_H
#define DLGPATHSEDITOR_H

#include <QDialog>

#include "shv/chainpack/rpcvalue.h"

#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/rpc/clientconnection.h>

namespace Ui {
class DlgPathsEditor;
}

class DlgPathsEditor : public QDialog
{
	Q_OBJECT

public:
	explicit DlgPathsEditor(QWidget *parent, shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_node_path);
	~DlgPathsEditor();

	void init(const QString &grant_id);
	QString pathId();
private:
	Ui::DlgPathsEditor *ui;
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
	std::string m_aclEtcPathsNodePath;

	void setPaths(const shv::chainpack::RpcValue::Map &paths);
	void callGetPaths();
};

#endif // DLGPATHSEDITOR_H
