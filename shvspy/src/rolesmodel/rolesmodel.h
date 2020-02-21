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

	void loadRoles(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path);

	void setSelectedRoles(const shv::chainpack::RpcValue::List &roles);
	shv::chainpack::RpcValue::List secetedRoles();

	Q_SIGNAL void rolesLoaded();
	Q_SIGNAL void loadError(QString error);

protected:
	virtual bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
	void loadChildItems(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path, const std::string &role_name);

private:
	void checkPartialySubRoles();
	void generateTree();
	void generateSubTree(QStandardItem *parent_item, const QString &role_name);
	void deleteRqId(int rqid);
	void createItems(const shv::chainpack::RpcValue::List &items);

	QSet<QString> allSubRoles();
	QSet<QString> flattenRole(QStandardItem *parent_item);
	QSet<QString> flattenRoleReverse(QStandardItem *child_item);

	QMap<QString, shv::chainpack::RpcValue::List> m_shvRoles;
	QVector<int> m_rqIds;
};



