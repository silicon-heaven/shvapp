#include "rolesmodel.h"

#include <shv/iotqt/rpc/rpcresponsecallback.h>

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
					/// pokud uz tento grant ma v ramci subgrantu, nenastavuj nic
					return ret;
				}
				else {
					bool check_on = (state == Qt::Checked);
					checkPartialySubGrants(it, check_on);
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
		sl << tr("Role") << tr("Note");
		setHorizontalHeaderLabels(sl);
	}

	QStandardItem *parent_item = invisibleRootItem();

	if (rpc_connection == nullptr)
		return;

	int rqid = rpc_connection->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(rpc_connection, rqid, this);

	cb->start(this, [this](const shv::chainpack::RpcResponse &response) {
		if(response.isValid()){
			if(response.isError()) {
				//ui->lblStatus->setText(tr("Failed to load roles.") + " " + QString::fromStdString(response.error().toString()));
			}
			else{
				if (response.result().isList()){
					shv::chainpack::RpcValue::List res = response.result().toList();

					for (size_t i = 0; i < res.size(); i++){
						QList<QStandardItem *> row;
						{
							QString role_name = QString::fromStdString(res.at(i).toStdString());
							QStandardItem *it = new QStandardItem(role_name);
							it->setData(role_name, NameRole);
				//			QStringList sl = qf::core::String(q.value("grants").toString()).splitAndTrim(',');
				//			it->setData(sl, GrantsRole);
							it->setCheckable(true);
							//it->setEnabled(TheApp::instance()->currentUserHasGrant(grant_name));
							loadChildItems(it);
							row << it;
						}
						{
							QStandardItem *it = new QStandardItem("caption");
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
	});

	rpc_connection->callShvMethod(rqid, acl_etc_roles_node_path, shv::chainpack::Rpc::METH_LS);
}

/*	qf::core::sql::Query q;
	q.exec("SELECT name, grants, caption FROM usergrants ORDER BY position");
	while(q.next()) {
		QString grant_name = q.value("name").toString();
		QString caption = q.value("caption").toString();
		if(caption.isEmpty())
			caption = grant_name;
		else
			caption = QCoreApplication::translate("GrantCaption", caption.toLatin1().constData());
		QList<QStandardItem *> row;
		{
			QStandardItem *it = new QStandardItem(grant_name);
			it->setData(grant_name, NameRole);
			QStringList sl = qf::core::String(q.value("grants").toString()).splitAndTrim(',');
			it->setData(sl, GrantsRole);
			it->setCheckable(true);
			it->setEnabled(TheApp::instance()->currentUserHasGrant(grant_name));
			loadChildItems(it);
			row << it;
		}
		{
			QStandardItem *it = new QStandardItem(caption);
			row << it;
		}
		parent_item->appendRow(row);
	}*/


void RolesTreeModel::loadChildItems(QStandardItem *parent_item)
{
/*	QStringList grant_list = parent_item->data(GrantsRole).toStringList();
	if(grant_list.isEmpty())
		return;
	QString grants = "('" + grant_list.join("','") + "')";
	QString qs = "SELECT name, grants, caption FROM usergrants WHERE name IN " + grants + " ORDER BY position";
	//qfInfo() << qs;
	qf::core::sql::Query q;
	q.exec(qs);
	while(q.next()) {
		QString name = q.value("name").toString();
		QString caption = q.value("caption").toString();
		if(caption.isEmpty())
			caption = name;
		else
			caption = QCoreApplication::translate("GrantCaption", caption.toLatin1().constData());
		QList<QStandardItem *> row;
		{
			QStandardItem *it = new QStandardItem(name);
			it->setData(name, NameRole);
			QStringList sl = qf::core::String(q.value("grants").toString()).splitAndTrim(',');
			it->setData(sl, GrantsRole);
			loadChildItems(it);
			row << it;
		}
		{
			QStandardItem *it = new QStandardItem(caption);
			row << it;
		}
		parent_item->appendRow(row);
	}*/
}

void RolesTreeModel::checkPartialySubGrants(QStandardItem *parent_item, bool check_on)
{
	for(int i=0; i<parent_item->rowCount(); i++) {
		QStandardItem *it = parent_item->child(i);
		QString grant = it->data(NameRole).toString();
		QStandardItem *root_item = invisibleRootItem();
		for(int i=0; i<root_item->rowCount(); i++) {
			QStandardItem *it1 = root_item->child(i);
			QString grant1 = it1->data(NameRole).toString();
			if(grant == grant1) {
				it1->setCheckState((check_on)?Qt::PartiallyChecked: Qt::Unchecked);
			}
		}
		checkPartialySubGrants(it, check_on);
	}
}
