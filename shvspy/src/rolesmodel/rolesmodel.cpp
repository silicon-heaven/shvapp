#include "rolesmodel.h"

#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/core/log.h>
#include <QDebug>

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
//							loadChildItems(rpc_connection, acl_etc_roles_node_path, it);
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

void RolesTreeModel::checkPartialySubRoles()
{
	QSet<QString> sub_roles = allSubRoles();
	QStandardItem *root_item = invisibleRootItem();

	for(int i = 0; i < root_item->rowCount(); i++) {
		QStandardItem *it = root_item->child(i);

		if (it->checkState() == Qt::CheckState::PartiallyChecked){
			if (!sub_roles.contains(it->data().toString())){
				it->setCheckState(Qt::CheckState::Unchecked);
			}
		}
		else{
			if (sub_roles.contains(it->data().toString())){
				it->setCheckState(Qt::CheckState::PartiallyChecked);
			}
		}
	}
}
