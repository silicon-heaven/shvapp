#pragma once

#include <shv/iotqt/rpc/clientconnection.h>

#include <QStandardItemModel>

class RolesTreeModel : public QStandardItemModel
{
public:
	enum Role {NameRole = Qt::UserRole+1, GrantsRole};
public:
	RolesTreeModel(QObject *parent = nullptr);
protected:
	void loadChildItems(QStandardItem *parent_item);
public:
	void checkPartialySubGrants(QStandardItem *parent_item, bool check_on);
	void loadRolesTree(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path);

	virtual bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
};



