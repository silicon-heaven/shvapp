#include "rolesmodel.h"

#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/core/log.h>

static const std::string VALUE_METHOD = "value";
static const std::string ROLES = "roles";

RolesTreeModel::RolesTreeModel(QObject * parent)
	: QStandardItemModel(parent)
{
}

bool RolesTreeModel::setData(const QModelIndex & ix, const QVariant & val, int role)
{
	bool ret = true;

	if(role == Qt::CheckStateRole) {
		ret = QStandardItemModel::setData(ix, val, role);

		int state = val.toInt();
		if (state != Qt::PartiallyChecked) {
				checkPartialySubRoles();
		}
	}
	else {
		ret = QStandardItemModel::setData(ix, val, role);
	}
	return ret;
}

void RolesTreeModel::loadRoles(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path)
{
	if (rpc_connection == nullptr)
		return;

	int rqid = rpc_connection->nextRequestId();
	m_rqIds.push_back(rqid);
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(rpc_connection, rqid, this);

	cb->start(this, [this, rqid, rpc_connection, acl_etc_roles_node_path](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
			if(response.isError()) {
				emit loadError(tr("Failed to load roles.") + " " + QString::fromStdString(response.error().toString()));
			}
			else{
				if (response.result().isList()){
					shv::chainpack::RpcValue::List roles = response.result().toList();

					for (size_t i = 0; i < roles.size(); i++){
						loadChildItems(rpc_connection, acl_etc_roles_node_path, roles.at(i).toStdString());
					}
				}
			}
		}
		else{
			emit loadError(tr("Request timeout expired for method: %1 path: %2").arg("ls").arg(QString::fromStdString(acl_etc_roles_node_path)));
		}

		deleteRqId(rqid);
	});

	rpc_connection->callShvMethod(rqid, acl_etc_roles_node_path, shv::chainpack::Rpc::METH_LS);
}

void RolesTreeModel::loadChildItems(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path, const std::string &role_name)
{
	if(rpc_connection == nullptr){
		return;
	}

	int rqid = rpc_connection->nextRequestId();
	m_rqIds.push_back(rqid);
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(rpc_connection, rqid, this);
	std::string role_path = acl_etc_roles_node_path + "/" + role_name;

	cb->start(this, [this, rqid, role_path, role_name](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if (response.isError()) {
				emit loadError(tr("Failed to load role: %1").arg(QString::fromStdString(role_name)) + " " + QString::fromStdString(response.error().toString()));
			}
			else{
				if (response.result().isMap()){
					shv::chainpack::RpcValue::Map res = response.result().toMap();
					shv::chainpack::RpcValue::List roles = res.value(ROLES).toList();
					m_shvRoles[QString::fromStdString(role_name)] = roles;
				}
			}
		}
		else{
			emit loadError(tr("Request timeout expired for method: %1 path: %2").arg(QString::fromStdString(VALUE_METHOD)).arg(QString::fromStdString(role_path)));
		}

		deleteRqId(rqid);
	});

	rpc_connection->callShvMethod(rqid, role_path, VALUE_METHOD);
}

void RolesTreeModel::checkRoles(const shv::chainpack::RpcValue::List &roles)
{
	for (size_t r = 0; r < roles.size(); r++){
		std::string role = roles.at(r).toStdString();
		QStandardItem *root_item = invisibleRootItem();

		for(int i=0; i < root_item->rowCount(); i++) {
			QStandardItem *it = root_item->child(i);
			bool check = (role == it->data().toString().toStdString());

			if (check){
				it->setCheckState(Qt::Checked);
			}
		}
	}

	checkPartialySubRoles();
}

shv::chainpack::RpcValue::List RolesTreeModel::checkedRoles()
{
	shv::chainpack::RpcValue::List roles;
	QStandardItem *root_item = invisibleRootItem();

	for(int i=0; i < root_item->rowCount(); i++) {
		QStandardItem *it = root_item->child(i);

		if (it->checkState() == Qt::CheckState::Checked){
			roles.push_back(it->data().toString().toStdString());
		}
	}

	return roles;
}

void RolesTreeModel::generateTree()
{
	clear();
	QStringList sl;
	sl << tr("Role");
	setHorizontalHeaderLabels(sl);

	QStandardItem *parent_item = invisibleRootItem();

	for (const QString &role_name: m_shvRoles.keys()){
		QList<QStandardItem *> row;
		QStandardItem *it = new QStandardItem(role_name);
		it->setData(role_name, NameRole);
		it->setCheckable(true);
		it->setFlags(it->flags() & ~Qt::ItemIsEditable);
		row << it;
		parent_item->appendRow(row);

		shv::chainpack::RpcValue::List sub_roles = m_shvRoles.value(role_name);
		for (size_t i = 0; i < sub_roles.size(); i++){
			generateSubTree(it, QString::fromStdString(sub_roles.at(i).toStdString()));
		}
	}
}

void RolesTreeModel::generateSubTree(QStandardItem *parent_item, const QString &role_name)
{
	if (flattenRoleReverse(parent_item).contains(role_name)){
		return;
	}

	QList<QStandardItem *> row;
	QStandardItem *it = new QStandardItem(role_name);
	it->setData(role_name, NameRole);
	it->setFlags(it->flags() & ~Qt::ItemIsEditable);
	row << it;
	parent_item->appendRow(row);

	shv::chainpack::RpcValue::List subRoles = m_shvRoles.value(role_name);

	for (size_t j = 0; j < subRoles.size(); j++){
		generateSubTree(it, QString::fromStdString(subRoles.at(j).toStdString()));
	}
}

void RolesTreeModel::deleteRqId(int rqid)
{
	for (int i = m_rqIds.count()-1; i >= 0; i--){
		if (m_rqIds.at(i) == rqid)
			m_rqIds.remove(i);
	}

	if (m_rqIds.isEmpty()){
		generateTree();

		emit rolesLoaded();
	}
}

QSet<QString> RolesTreeModel::allSubRoles()
{
	QSet<QString> roles;
	QStandardItem *root_item = invisibleRootItem();

	for (int i = 0; i < root_item->rowCount(); i++) {
		QStandardItem *it = root_item->child(i);

		if (it->checkState() == Qt::CheckState::Checked){
			for(int ch = 0; ch < it->rowCount(); ch++) {
				QStandardItem *it1 = it->child(ch);
				roles += flattenRole(it1);
			}
		}
	}

	return roles;
}

QSet<QString> RolesTreeModel::flattenRole(QStandardItem *parent_item)
{
	QSet<QString> ret;

	ret.insert(parent_item->data().toString());

	for (int i = 0; i < parent_item->rowCount(); i++) {
		QStandardItem *it = parent_item->child(i);
		QString role_name = it->data().toString();

		if(!ret.contains(role_name)) {
			ret += flattenRole(it);
		}
	}

	return ret;
}

QSet<QString> RolesTreeModel::flattenRoleReverse(QStandardItem *child_item)
{
	QSet<QString> ret;

	if (child_item != nullptr && child_item != invisibleRootItem()){
		QString role_name = child_item->data().toString();

		if(!ret.contains(role_name)) {
			ret.insert(role_name);
			ret += flattenRoleReverse(child_item->parent());
		}
	}

	return ret;
}

void RolesTreeModel::checkPartialySubRoles()
{
	QSet<QString> selected_sub_roles = allSubRoles();
	QStandardItem *root_item = invisibleRootItem();

	for(int i = 0; i < root_item->rowCount(); i++) {
		QStandardItem *it = root_item->child(i);

		if (it->checkState() == Qt::CheckState::PartiallyChecked){
			if (!selected_sub_roles.contains(it->data().toString())){
				it->setCheckState(Qt::CheckState::Unchecked);
			}
		}
		else{
			if (selected_sub_roles.contains(it->data().toString())){
				it->setCheckState(Qt::CheckState::PartiallyChecked);
			}
		}
	}
}
