#include "shvnodeitem.h"
#include "shvbrokernodeitem.h"
#include "servertreemodel.h"

#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/cponwriter.h>
#include <src/theapp.h>

#include <shv/core/assert.h>

#include <QIcon>
#include <QVariant>

namespace cp = shv::chainpack;

std::string ShvMetaMethod::signatureStr() const
{
	std::string ret;
	switch (signature) {
	case cp::MetaMethod::Signature::VoidVoid: ret = "void()"; break;
	case cp::MetaMethod::Signature::VoidParam: ret = "void(param)"; break;
	case cp::MetaMethod::Signature::RetVoid: ret = "ret()"; break;
	case cp::MetaMethod::Signature::RetParam: ret = "ret(param)"; break;
	}
	//if(isSignal)
	//	ret = ret + " NTF";
	return ret;
}

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

void ShvNodeItem::deleteChild(int ix)
{
	ShvNodeItem *ret = childAt(ix);
	if(ret) {
		ServerTreeModel *m = treeModel();
		m->beginRemoveRows(m->indexFromItem(this), ix, ix);
		m_children.remove(ix);
		delete ret;
		m->endRemoveRows();
	}
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
		if(!nd || nd == srv_nd) {
			break;
		}
		else {
			if(!ret.empty())
				ret = '/' + ret;
			ret = nd->nodeId() + ret;
		}
		nd = nd->parentNode();
	}
	return ret;
}

void ShvNodeItem::processRpcMessage(const shv::chainpack::RpcMessage &msg)
{
	if(msg.isResponse()) {
		cp::RpcResponse resp(msg);
		unsigned rqid = resp.requestId().toUInt();
		if(rqid == m_loadChildrenRqId) {
			m_loadChildrenRqId = 0;
			m_childrenLoaded = true;

			deleteChildren();
			ServerTreeModel *m = treeModel();
			const shv::chainpack::RpcValue::List lst = resp.result().toList();
			for(const cp::RpcValue &dir_entry : lst) {
				const cp::RpcValue::List &long_dir_entry = dir_entry.toList();
				std::string ndid = long_dir_entry.empty()? dir_entry.toString(): long_dir_entry.value(0).toString();
				ShvNodeItem *nd = new ShvNodeItem(m, ndid);
				if(!long_dir_entry.empty()) {
					cp::RpcValue has_children = long_dir_entry.value(1);
					if(has_children.isBool()) {
						nd->setHasChildren(has_children.toBool());
						if(!has_children.toBool())
							nd->setChildrenLoaded();
					}
				}
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
				cp::RpcValueGenList lst(v);
				mm.method = lst.value(0).toString();
				mm.signature = (cp::MetaMethod::Signature) lst.value(1).toUInt();
				mm.isNotify = lst.value(2).toBool();
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
	m_loadChildrenRqId = srv_nd->callNodeRpcMethod(shvPath(), "ls", cp::RpcValue::List{std::string(), (unsigned)0x7F});
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
	m_loadMethodsRqId = srv_nd->callNodeRpcMethod(shvPath(), "dir", cp::RpcValue::List{std::string(), 127});
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
	if(mtd.method.empty() || mtd.isNotify)
		return 0;
	if (mtd.params.isValid()) {
		TheApp::instance()->addLastUsedParam(
					QString::fromStdString(shvPath()),
					QString::fromStdString(mtd.method),
					QString::fromStdString(mtd.params.toCpon())
					);
	}
	mtd.response = cp::RpcResponse();
	ShvBrokerNodeItem *srv_nd = serverNode();
	mtd.rpcRequestId = srv_nd->callNodeRpcMethod(shvPath(), mtd.method, mtd.params);
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

