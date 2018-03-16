#include "servertreemodel.h"
#include "shvbrokernodeitem.h"

#include "../theapp.h"

#include <shv/core/utils.h>
#include <shv/coreqt/log.h>
#include <shv/core/assert.h>

#include <QSettings>
#include <QJsonDocument>
#include <QJsonParseError>

ServerTreeModel::ServerTreeModel(QObject *parent)
	: Super(parent)
{
	m_invisibleRoot = new ShvNodeRootItem(this);
}

ServerTreeModel::~ServerTreeModel()
{
}

QModelIndex ServerTreeModel::index(int row, int column, const QModelIndex &parent) const
{
	ShvNodeItem *pnd = itemFromIndex(parent);
	if(!pnd || column > 0)
		return QModelIndex();
	ShvNodeItem *nd = pnd->childAt(row);
	return createIndex(row, 0, nd->modelId());
}

ShvBrokerNodeItem *ServerTreeModel::createConnection(const QVariantMap &params)
{
	ShvBrokerNodeItem *ret = new ShvBrokerNodeItem(nextId(), params.value("name").toString().toStdString());
	ret->setServerProperties(params);
	//ret->setOid(oid);
	invisibleRootItem()->appendChild(ret);
	return ret;
}
/*
ServerNode *ServerTreeModel::connectionForOid(int oid)
{
	ServerNode *ret = nullptr;
	QStandardItem *root = invisibleRootItem();
	for(int i=0; i<root->rowCount(); i++) {
		ServerNode *nd = dynamic_cast<ServerNode*>(root->child(i));
		SHV_ASSERT_EX(nd != nullptr, "Internal error");
		if(nd->oid() == oid) {
			ret = nd;
			break;
		}
	}
	return ret;
}
*/
int ServerTreeModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent);
	return 1;
}

int ServerTreeModel::rowCount(const QModelIndex &parent) const
{
	QStandardItem *par_it = itemFromIndex(parent);
	shvDebug() << "ServerTreeModel::rowCount, item:" << par_it << "parent model index valid:" << parent.isValid();
	ShvNodeItem *par_nd = dynamic_cast<ShvNodeItem*>(par_it);
	if(par_nd) {
		if(par_nd->nodeId() == "localhost")
			par_nd->shvPath();
		shvDebug() << "\t parent node:" << par_nd << "id:" << par_nd->nodeId() << "path:" << par_nd->shvPath();
		if(!par_nd->isChildrenLoaded() && !par_nd->isChildrenLoading()) {
			shvDebug() << "l\t oading" << par_nd->shvPath();
			par_nd->loadChildren();
		}
	}
	int rcnt = Super::rowCount(parent);
	shvDebug() << "\t return:" << rcnt;
	return rcnt;
}

QVariant ServerTreeModel::data(const QModelIndex &ix, int role) const
{
	return Super::data(ix, role);
}

QVariant ServerTreeModel::headerData(int section, Qt::Orientation o, int role) const
{
	return Super::headerData(section, o, role);
}

ShvNodeItem *ServerTreeModel::itemFromIndex(const QModelIndex &ix) const
{
	if(!ix.isValid())
		return m_invisibleRoot;
	ShvNodeItem *nd = m_nodes.value(ix.internalId());
	return nd;
}

void ServerTreeModel::loadSettings(const QSettings &settings)
{
	QString servers_json = settings.value("application/servers").toString();
	QJsonParseError err;
	QJsonDocument jsd = QJsonDocument::fromJson(servers_json.toUtf8(), &err);
	if(err.error != QJsonParseError::NoError) {
		shvError() << "Erorr parse server settings:" << err.errorString();
		return;
	}
	const QVariantList lst = jsd.toVariant().toList();
	for(const auto v : lst) {
		QVariantMap m = v.toMap();
		m["password"] = QString::fromStdString(TheApp::instance()->crypt().decrypt(m.value("password").toString().toStdString()));
		createConnection(m);
	}
}

void ServerTreeModel::saveSettings(QSettings &settings)
{
	QStandardItem *root = invisibleRootItem();
	QVariantList lst;
	for(int i=0; i<root->rowCount(); i++) {
		ShvBrokerNodeItem *nd = dynamic_cast<ShvBrokerNodeItem*>(root->child(i));
		SHV_ASSERT_EX(nd != nullptr, "Internal error");
		QVariantMap props = nd->serverProperties();
		props["password"] = QString::fromStdString(TheApp::instance()->crypt().encrypt(props.value("password").toString().toStdString(), 30));
		lst << props;
	}
	QJsonDocument jsd = QJsonDocument::fromVariant(lst);
	QString servers_json = QString::fromUtf8(jsd.toJson(QJsonDocument::Compact));
	settings.setValue("application/servers", servers_json);
}
