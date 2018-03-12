#include "shvnodeitem.h"
#include "servernode.h"

//#include <qfopcua/client.h>
//#include <qfopcua/datavalue.h>

#include <shv/core/assert.h>

ShvNodeItem::ShvNodeItem(const std::string &ndid)
	: Super(), m_nodeId(ndid)
{
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
		/*
		QVariant v = attribute(qfopcua::AttributeId::NodeClass);
		qfopcua::NodeClass::Enum nc = qfopcua::NodeClass::fromInt(v.toInt());
		static QIcon object = QIcon(QStringLiteral(":/qfopcuaspy/images/object"));
		static QIcon variable = QIcon(QStringLiteral(":/qfopcuaspy/images/variable"));
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

ServerNode *ShvNodeItem::serverNode() const
{
	const ServerNode *ret = nullptr;
	for(const QStandardItem *it = this; it; it=it->parent()) {
		ret = dynamic_cast<const ServerNode*>(it);
		if(ret)
			break;
	}
	SHV_ASSERT_EX(ret != nullptr, "ServerNode parent must exist.");
	return const_cast<ServerNode*>(ret);
}

void ShvNodeItem::loadChildren(bool force)
{
	if(!m_childrenLoaded || force) {
		removeRows(0, rowCount());
		m_childrenLoaded = true;
		/*
		const qfopcua::NodeIdList lst = serverNode()->clientConnection()->getChildren(m_nodeId);
		for(const auto &ndid : lst) {
			QStandardItem *it = new ShvNodeItem(ndid);
			appendRow(it);
		}
		*/
	}
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
