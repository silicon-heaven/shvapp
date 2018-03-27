#include "shvnodeitem.h"
#include "shvbrokernodeitem.h"
#include "servertreemodel.h"

#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/cponwriter.h>

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
			const shv::chainpack::RpcValue::List lst = resp.result().toList();
			for (size_t i = 0; i < lst.size(); i+=2) {
				const cp::RpcValue &ndid = lst.value(i);
				ShvNodeItem *nd = new ShvNodeItem(m, ndid.toString());
				const cp::RpcValue::List &dir = lst.value(i+1).toList();
				bool has_children = !(std::find(dir.begin(), dir.end(), std::string("ls")) == dir.end());
				if(!has_children)
					nd->setChildrenLoaded();
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
				mm.method = v.toString();
				m_methods.push_back(mm);
			}
			emit methodsLoaded();
		}
		else {
			for (int i = 0; i < m_methods.count(); ++i) {
				ShvMetaMethod &mtd = m_methods[i];
				if(mtd.rpcRequestId == rqid) {
					mtd.rpcRequestId = 0;
					mtd.response = resp;
					emit rpcMethodCallFinished(i);
					break;
				}
			}
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
	m_loadChildrenRqId = srv_nd->callShvMethod(shvPath(), "ls", "dir");
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

void ShvNodeItem::setMethodParams(int method_ix, const shv::chainpack::RpcValue &params)
{
	if(method_ix < 0 || method_ix >= m_methods.count())
		return;
	ShvMetaMethod &mtd = m_methods[method_ix];
	mtd.params = params;
}

unsigned ShvNodeItem::callMethod(int method_ix)
{
	//const QVector<ShvMetaMethod> &mm = m_methods();
	if(method_ix < 0 || method_ix >= m_methods.count())
		return 0;
	ShvMetaMethod &mtd = m_methods[method_ix];
	if(mtd.method.empty())
		return 0;
	/*
	cp::RpcValue params;
	if(!mtd.params.empty()) {
		std::istringstream is(mtd.params);
		try {
			cp::CponReader rd(is);
			rd >> params;
		}
		catch (cp::CponReader::ParseException &e) {
			shvError() << "error parsing params:" << e.mesage();
		}
	}
	*/
	mtd.response = cp::RpcResponse();
	ShvBrokerNodeItem *srv_nd = serverNode();
	mtd.rpcRequestId = srv_nd->callShvMethod(shvPath(), mtd.method, mtd.params);
	return mtd.rpcRequestId;
}

void ShvNodeItem::reload()
{
	deleteChildren();
	m_methods.clear();
	loadChildren();
	loadMethods();
	emitDataChanged();
}

ShvNodeRootItem::ShvNodeRootItem(ServerTreeModel *parent)
	: Super(parent, std::string(), nullptr)
{
	setParent(parent);
}
