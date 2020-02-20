#pragma once

#include <shv/iotqt/rpc/clientconnection.h>

#include <QStandardItemModel>
#include <QSet>

class RolesTreeModel : public QStandardItemModel
{
	Q_OBJECT
public:
	enum Role {NameRole = Qt::UserRole+1};
	RolesTreeModel(QObject *parent = nullptr);

	void checkPartialySubRoles();
	void loadRolesTree(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path);
	void setCheckedRoles(const shv::chainpack::RpcValue::List &roles);

	shv::chainpack::RpcValue::List checkedRoles();

	virtual bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );

	Q_SIGNAL void treeLoaded();

protected:
	void loadChildItems(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path, QStandardItem *parent_item);

private:
	QVector<int> m_rqIds;

	void deleteRqId(int rqid);
	void createItems(const shv::chainpack::RpcValue::List &items);
	QSet<QString> allSubRoles();
	QSet<QString> flattenRole(QStandardItem *parent_item);


};



