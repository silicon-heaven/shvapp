#include "shvnodeitem.h"
#include "shvbrokernodeitem.h"

#include <shv/iotqt/rpc/clientconnection.h>
//#include <qfopcua/datavalue.h>

#include <shv/core/assert.h>

namespace cp = shv::chainpack;

ShvNodeItem::ShvNodeItem(unsigned model_id, const std::string &ndid, QObject *parent)
	: Super(parent)
	, m_nodeId(ndid)
	, m_modelId(model_id)
{
	setObjectName(QString::fromStdString(nodeid));
}

ShvNodeItem::~ShvNodeItem()
{
}

QVariant ShvNodeItem::data(int role) const
{
	QVariant ret = Super::data(role);
	if(role == Qt::DisplayRole) {
		//qfopcua::QualifiedName qn = attribute(qfopcua::AttributeId::BrowseName).value<qfopcua::QualifiedName>();
		//ret = qn.text();
	}
	else if(role == Qt::DecorationRole) {
		if(isChildrenLoading()) {
			static QIcon ico_reload = QIcon(QStringLiteral(":/shvspy/images/reload"));
			ret = ico_reload;
		}
		/*
		QVariant v = attribute(qfopcua::AttributeId::NodeClass);
		qfopcua::NodeClass::Enum nc = qfopcua::NodeClass::fromInt(v.toInt());
		static QIcon object = QIcon(QStringLiteral(":/shvspy/images/object"));
		static QIcon variable = QIcon(QStringLiteral(":/shvspy/images/variable"));
		switch(nc) {
		case qfopcua::NodeClass::Object:
			ret = object;
			break;
		case qfopcua::NodeClass::Variable:
			ret = variable;
			break;
		default:
			break;
		}
		*/
	}
	return ret;
}

ShvBrokerNodeItem *ShvNodeItem::serverNode() const
{
	ShvBrokerNodeItem *ret = nullptr;
	for(QStandardItem *it = const_cast<ShvNodeItem*>(this); it; it=it->parent()) {
		ret = dynamic_cast<ShvBrokerNodeItem*>(it);
		if(ret)
			break;
	}
	SHV_ASSERT_EX(ret != nullptr, "ServerNode parent must exist.");
	return ret;
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
	m_children.insert(ix, n);
}

ShvNodeItem *ShvNodeItem::takeChild(int ix)
{
	ShvNodeItem *ret = childAt(ix);
	ret->setParent(nullptr);
	m_children.remove(ix);
	return ret;
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
		nd = dynamic_cast<ShvNodeItem*>(nd->Super::parent());
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

			removeRows(0, rowCount());
			for(const cp::RpcValue &ndid : resp.result().toList()) {
				QStandardItem *it = new ShvNodeItem(ndid.toString());
				it->appendRow(new QStandardItem("kkt"));
				appendRow(it);
			}
		}
	}
}

void ShvNodeItem::loadChildren()
{
	m_childrenLoaded = false;
	ShvBrokerNodeItem *srv_nd = serverNode();
	m_loadChildrenRqId = srv_nd->requestLoadChildren(shvPath());
	emitDataChanged();
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
