#include "shvnodeitem.h"
#include "shvbrokernodeitem.h"
#include "servertreemodel.h"

#include <shv/iotqt/rpc/clientconnection.h>
//#include <qfopcua/datavalue.h>

#include <shv/core/assert.h>

#include <QIcon>
#include <QVariant>

namespace cp = shv::chainpack;

ShvNodeItem::ShvNodeItem(ServerTreeModel *m, const std::string &ndid, ShvNodeItem *parent)
	: Super(parent)
	, m_nodeId(ndid)
	, m_treeModelId(m->nextId())
{
	setObjectName(QString::fromStdString(ndid));
	m->m_nodes[m_treeModelId] = this;
}

ShvNodeItem::~ShvNodeItem()
{
	treeModel()->m_nodes.remove(m_treeModelId);
}

ServerTreeModel *ShvNodeItem::treeModel() const
{
	for(QObject *o = this->parent(); o; o = o->parent()) {
		ServerTreeModel *m = qobject_cast<ServerTreeModel*>(o);
		if(m)
			return m;
	}
	SHV_EXCEPTION("ServerTreeModel parent must exist.");
	return nullptr;
}

QVariant ShvNodeItem::data(int role) const
{
	QVariant ret;
	if(role == Qt::DisplayRole) {
		ret = objectName();
	}
	else if(role == Qt::DecorationRole) {
		if(isChildrenLoading()) {
			static QIcon ico_reload = QIcon(QStringLiteral(":/shvspy/images/reload"));
			ret = ico_reload;
		}
	}
	return ret;
}

ShvBrokerNodeItem *ShvNodeItem::serverNode() const
{
	for(ShvNodeItem *nd = const_cast<ShvNodeItem*>(this); nd; nd=nd->parentNode()) {
		ShvBrokerNodeItem *bnd = qobject_cast<ShvBrokerNodeItem *>(nd);
		if(bnd)
			return bnd;
	}
	SHV_EXCEPTION("ServerNode parent must exist.");
	return nullptr;
}

ShvNodeItem *ShvNodeItem::parentNode() const
{
	return qobject_cast<ShvNodeItem*>(parent());
}

ShvNodeItem *ShvNodeItem::childAt(int ix) const
{
	if(ix < 0 || ix >= m_children.count())
		SHV_EXCEPTION("Invalid child index");
	return m_children[ix];
}

void ShvNodeItem::insertChild(int ix, ShvNodeItem *n)
{
	ServerTreeModel *m = treeModel();
	m->beginInsertRows(m->indexFromItem(this), ix, ix);
	n->setParent(this);
	m_children.insert(ix, n);
	m->endInsertRows();
}

ShvNodeItem *ShvNodeItem::takeChild(int ix)
{
	ShvNodeItem *ret = childAt(ix);
	if(ret) {
		ServerTreeModel *m = treeModel();
		m->beginRemoveRows(m->indexFromItem(this), ix, ix);
		ret->setParent(nullptr);
		m_children.remove(ix);
		m->endRemoveRows();
	}
	return ret;
}

void ShvNodeItem::deleteChildren()
{
	if(childCount() == 0)
		return;
	ServerTreeModel *m = treeModel();
	m->beginRemoveRows(m->indexFromItem(this), 0, childCount() - 1);
	qDeleteAll(m_children);
	m_children.clear();
	m->endRemoveRows();
	emitDataChanged();
}

std::string ShvNodeItem::shvPath() const
{
	std::string ret;
	ShvBrokerNodeItem *srv_nd = serverNode();
	const ShvNodeItem *nd = this;
	while(nd) {
		ret = '/' + ret;
		if(nd != srv_nd)
			ret = nd->nodeId() + ret;
		nd = nd->parentNode();
	}
	return ret;
}

void ShvNodeItem::processRpcMessage(const shv::chainpack::RpcMessage &msg)
{
	if(msg.isResponse()) {
		cp::RpcResponse resp(msg);
		unsigned rqid = resp.requestId();
		if(rqid == m_loadChildrenRqId) {
			m_loadChildrenRqId = 0;
			m_childrenLoaded = true;

			deleteChildren();
			ServerTreeModel *m = treeModel();
			for(const cp::RpcValue &ndid : resp.result().toList()) {
				ShvNodeItem *nd = new ShvNodeItem(m, ndid.toString());
				appendChild(nd);
			}
			emitDataChanged();
		}
		else if(rqid == m_loadMethodsRqId) {
			m_loadMethodsRqId = 0;
			m_methodsLoaded = true;

			m_methods.clear();
			for(const cp::RpcValue &v : resp.result().toList()) {
				ShvMetaMethod mm;
				mm.setName(v.toString());
				m_methods.push_back(mm);
			}
			emit methodsLoaded();
		}
	}
}

void ShvNodeItem::emitDataChanged()
{
	ServerTreeModel *m = treeModel();
	QModelIndex ix = m->indexFromItem(this);
	emit m->dataChanged(ix, ix);
}

void ShvNodeItem::loadChildren()
{
	m_childrenLoaded = false;
	ShvBrokerNodeItem *srv_nd = serverNode();
	m_loadChildrenRqId = srv_nd->callShvMethod(shvPath(), "ls", cp::RpcValue());
	emitDataChanged();
}

bool ShvNodeItem::checkMethodsLoaded()
{
	if(!isMethodsLoaded() && !isMethodsLoading()) {
		loadMethods();
		return false;
	}
	return true;
}

void ShvNodeItem::loadMethods()
{
	m_methodsLoaded = false;
	ShvBrokerNodeItem *srv_nd = serverNode();
	m_loadMethodsRqId = srv_nd->callShvMethod(shvPath(), "dir", cp::RpcValue());
	//emitDataChanged();
}
/*
QVariant ShvNodeItem::attribute(qfopcua::AttributeId::Enum attr_id) const
{
	shvLogFuncFrame() << "att_id:" << qfopcua::AttributeId::toString(attr_id);
	if(!m_attribudes.contains(attr_id)) {
		ServerNode *srvnd = serverNode();
		qfopcua::DataValue dv = srvnd->clientConnection()->getAttribute(m_nodeId, attr_id);
		qfDebug() << dv.toString();
		m_attribudes[attr_id] = dv.value();
	}
	QVariant ret = m_attribudes.value(attr_id);
	return ret;
}
*/

ShvNodeRootItem::ShvNodeRootItem(ServerTreeModel *parent)
	: Super(parent, std::string(), nullptr)
{
	setParent(parent);
}
