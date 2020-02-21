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

	void setSelectedRoles(const std::vector<std::string> &roles);
	std::vector<std::string> selectedRoles();

	Q_SIGNAL void loadRolesFinished();
	Q_SIGNAL void loadError(QString error);

protected:
	virtual bool setData ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole);
	void loadChildItems(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path, QVector<std::string> role_names);

private:
	void checkPartialySubRoles();
	void generateTree();
	void generateSubTree(QStandardItem *parent_item, const QString &role, QSet<QString> created_roles);
	void createItems(const shv::chainpack::RpcValue::List &items);

	QSet<QString> allSubRoles();
	QSet<QString> flattenRole(QStandardItem *parent_item);

	QMap<QString, std::vector<std::string>> m_shvRoles;
};



