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
		int state = val.toInt();
		if(state != Qt::PartiallyChecked) {
			QStandardItem *it = itemFromIndex(ix);
			if(it) {
				if(it->checkState() == Qt::PartiallyChecked) {
					return ret;
				}
				else {
					bool check_on = (state == Qt::Checked);
					//checkAllPartialySubRoles();
					checkPartialySubRoles(it, check_on);
				}
			}
		}
		ret = QStandardItemModel::setData(ix, val, role);
	}
	else {
		ret = QStandardItemModel::setData(ix, val, role);
	}
	return ret;
}

void RolesTreeModel::loadRolesTree(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path)
{
	clear();
	{
		QStringList sl;
		sl << tr("Role");
		setHorizontalHeaderLabels(sl);
	}

	QStandardItem *parent_item = invisibleRootItem();

	if (rpc_connection == nullptr)
		return;

	int rqid = rpc_connection->nextRequestId();
	m_rqIds.push_back(rqid);
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(rpc_connection, rqid, this);

	cb->start(this, [this, rqid, rpc_connection, acl_etc_roles_node_path](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
			if(response.isError()) {
				//ui->lblStatus->setText(tr("Failed to load roles.") + " " + QString::fromStdString(response.error().toString()));
			}
			else{
				if (response.result().isList()){
					shv::chainpack::RpcValue::List roles = response.result().toList();
					for (size_t i = 0; i < roles.size(); i++){
						QList<QStandardItem *> row;
						{
							QString role_name = QString::fromStdString(roles.at(i).toStdString());
							QStandardItem *it = new QStandardItem(role_name);
							it->setData(role_name, NameRole);
							it->setCheckable(true);
							it->setFlags(it->flags() & ~Qt::ItemIsEditable);
							loadChildItems(rpc_connection, acl_etc_roles_node_path, it);
							row << it;
						}
						QStandardItem *parent_item = invisibleRootItem();
						parent_item->appendRow(row);
					}
				}
			}
		}
		else{
			//ui->lblStatus->setText(tr("Request timeout expired"));
		}

		deleteRqId(rqid);
	});

	rpc_connection->callShvMethod(rqid, acl_etc_roles_node_path, shv::chainpack::Rpc::METH_LS);
}

void RolesTreeModel::setCheckedRoles(const shv::chainpack::RpcValue::List &roles)
{
	for (int r = 0; r < roles.size(); r++){
		std::string role = roles.at(r).toStdString();
		QStandardItem *root_item = invisibleRootItem();

		for(int i=0; i < root_item->rowCount(); i++) {
			QStandardItem *it = root_item->child(i);
			bool has_role = (role == it->data().toString().toStdString());
			if (has_role){
				it->setCheckState(Qt::Checked);
				checkPartialySubRoles(it, true);
			}
		}
	}
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

void RolesTreeModel::loadChildItems(shv::iotqt::rpc::ClientConnection *rpc_connection, const std::string &acl_etc_roles_node_path, QStandardItem *parent_item)
{
	if(rpc_connection == nullptr){
		return;
	}

	int rqid = rpc_connection->nextRequestId();
	m_rqIds.push_back(rqid);
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(rpc_connection, rqid, this);

	cb->start(this, [this, rqid, rpc_connection, acl_etc_roles_node_path, parent_item](const shv::chainpack::RpcResponse &response) {
		if (response.isValid()){
			if (response.isError()) {
				//ui->lblStatus->setText(tr("Failed to call method ls.") + QString::fromStdString(response.error().toString()));
			}
			else{
				if (response.result().isMap()){
					shv::chainpack::RpcValue::Map res = response.result().toMap();
					shv::chainpack::RpcValue::List roles = res.value(ROLES).toList();

					for (size_t i = 0; i < roles.size(); i++){
						QList<QStandardItem *> row;
						{
							QString role_name = QString::fromStdString(roles.at(i).toStdString());
							QStandardItem *it = new QStandardItem(role_name);
							it->setData(role_name, NameRole);
							it->setFlags(it->flags() & ~Qt::ItemIsEditable);
							loadChildItems(rpc_connection, acl_etc_roles_node_path, it);
							row << it;
						}
						parent_item->appendRow(row);
					}
				}
			}
		}
		else{
			//ui->lblStatus->setText(tr("Request timeout expired"));
		}

		deleteRqId(rqid);
	});

	std::string role_path = acl_etc_roles_node_path + "/" + parent_item->data().toString().toStdString();
	rpc_connection->callShvMethod(rqid, role_path, VALUE_METHOD);
}

void RolesTreeModel::deleteRqId(int rqid)
{
	for (int i = m_rqIds.count()-1; i >= 0; i--){
		if (m_rqIds.at(i) == rqid)
			m_rqIds.remove(i);
	}

	if (m_rqIds.isEmpty()){
		emit treeLoaded();
	}
}

bool RolesTreeModel::checkIfRoleIsSet(const std::string &role)
{
	bool is_set = false;

	QStandardItem *root_item = invisibleRootItem();

	for (int i = 0; i < root_item->rowCount(); i++) {
		if (root_item->child(i)->checkState() == Qt::CheckState::Checked)
			is_set |= isRoleSet(root_item->child(i), role);
	}

	return is_set;
}

bool RolesTreeModel::isRoleSet(QStandardItem *parent_item, const std::string &role)
{
	bool is_set = false;

	for(int i = 0; i < parent_item->rowCount(); i++) {
		QStandardItem *it = parent_item->child(i);
		std::string role1 = it->data(NameRole).toString().toStdString();

		if(role == role1) {
			return true;
		}
		else{
			is_set |= isRoleSet(it, role);
		}
	}

	return is_set;
}

void RolesTreeModel::checkAllPartialySubRoles()
{
	QStandardItem *root_item = invisibleRootItem();

	for(int i = 0; i < root_item->rowCount(); i++) {
		QStandardItem *it = root_item->child(i);

		if (it->checkState() == Qt::PartiallyChecked){
			it->setCheckState(Qt::Unchecked);
		}
	}
}

void RolesTreeModel::checkPartialySubRoles(QStandardItem *parent_item, bool check_on)
{
	for(int i = 0; i < parent_item->rowCount(); i++) {
		QStandardItem *it = parent_item->child(i);
		QString role = it->data(NameRole).toString();
		QStandardItem *root_item = invisibleRootItem();

		for(int i = 0; i < root_item->rowCount(); i++) {
			QStandardItem *it1 = root_item->child(i);
			QString role1 = it1->data(NameRole).toString();

			if(role == role1) {
				it1->setCheckState((check_on) ? Qt::PartiallyChecked: Qt::Unchecked);
			}
		}
		checkPartialySubRoles(it, check_on);
	}
}
