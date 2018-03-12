#include "attributesmodel.h"
#include "attributenode.h"
#include "nodeidattributenode.h"
#include "datavalueattributenode.h"
#include "valueattributenode.h"
#include "accesslevelattributenode.h"
#include "nodeclassattributenode.h"
#include "qualifiednameattributenode.h"

#include "../theapp.h"

#include <shv/core/utils.h>
#include <shv/coreqt/log.h>
#include <shv/core/assert.h>

#include <QSettings>
#include <QJsonDocument>
#include <QJsonParseError>

AttributesModel::AttributesModel(QObject *parent)
	: Super(parent)
{
}

AttributesModel::~AttributesModel()
{
}

Qt::ItemFlags AttributesModel::flags(const QModelIndex &ix) const
{
	bool editable = false;
	if(ix.column() == 1) {
		ValueAttributeNode *nd = dynamic_cast<ValueAttributeNode*>(itemFromIndex(ix.sibling(ix.row(), 0)));
		//shvInfo() << ix.row() << nd << m_userAccessLevel;
		if(nd) {
			//editable = (m_userAccessLevel & qfopcua::AccessLevel::CurrentWrite);
		}
	}
	Qt::ItemFlags ret = Super::flags(ix);
	if(editable)
		ret |= Qt::ItemIsEditable;
	else {
		ret &= ~Qt::ItemIsEditable;
	}
	return ret;
}

QVariant AttributesModel::data(const QModelIndex &ix, int role) const
{
	QVariant ret;
	AttributeNodeBase *nd = dynamic_cast<AttributeNodeBase*>(itemFromIndex(ix.sibling(ix.row(), 0)));
	SHV_ASSERT(nd != nullptr, QString("Internal error ix(%1, %2) %3").arg(ix.row()).arg(ix.column()).arg(ix.internalId()), return QVariant());
	if(ix.column() == 0) {
		if(role == Qt::DisplayRole)
			ret = nd->name();
		else
			ret = Super::data(ix, role);
	}
	else if(ix.column() == 1) {
		if(role == Qt::DisplayRole)
			ret = nd->displayValue();
		else if(role == Qt::ToolTipRole)
			ret = nd->displayValue();
		else if(role == Qt::EditRole)
			ret = nd->toEditorValue(nd->value());
		else
			ret = Super::data(ix, role);
	}
	return ret;
}

bool AttributesModel::setData(const QModelIndex &ix, const QVariant &val, int role)
{
	shvLogFuncFrame() << val.toString() << val.typeName() << "role:" << role;
	bool ret = false;
	if(role == Qt::EditRole) {
		ValueAttributeNode *nd = dynamic_cast<ValueAttributeNode*>(itemFromIndex(ix.sibling(ix.row(), 0)));
		if(nd) {
			QVariant val_to_set = val;
			AttributeNode *pnd = dynamic_cast<AttributeNode*>(nd->parent());
			ValueAttributeNode *vnd = dynamic_cast<ValueAttributeNode*>(nd->parent());
			if(vnd) {
				// set array value
				QVariantList arr;
				for (int i = 0; i < vnd->rowCount(); ++i) {
					ValueAttributeNode *chnd = dynamic_cast<ValueAttributeNode*>(vnd->child(i));
					SHV_ASSERT(chnd != nullptr, "Bad child.", return false);
					if(i == ix.row())
						arr << chnd->fromEditorValue(val);
					else
						arr << chnd->value();
				}
				val_to_set = arr;
				pnd = dynamic_cast<AttributeNode*>(vnd->parent());
			}
			else {
				val_to_set = nd->fromEditorValue(val_to_set);
			}
			/*
			SHV_ASSERT(pnd != nullptr, "Bad parent, should be type of AttributeNode.", return false);
			qfopcua::NodeId ndid = pnd->attributesModel()->nodeId();
			int type = pnd->value().userType();
			shvInfo() << ndid.toString() << "retyping" << val_to_set << "to node type:" << QMetaType::typeName(pnd->value().userType());
			val_to_set.convert(type);
			shvInfo() << "retyped" << val_to_set;
			ret = m_client->setAttribute(ndid, pnd->attributeId(), val_to_set);
			if(ret) {
				pnd->load(true);
			}
			else {
				qfError() << "Set attribute error:" << m_client->errorString();
			}
			*/
		}
	}
	return ret;
}

QVariant AttributesModel::headerData(int section, Qt::Orientation o, int role) const
{
	QVariant ret;
	if(o == Qt::Horizontal) {
		if(role == Qt::DisplayRole) {
			if(section == 0)
				ret = tr("Attribute");
			else if(section == 1)
				ret = tr("Value");
		}
	}
	return ret;
}
/*
void AttributesModel::setNode(qfopcua::Client *client, const qfopcua::NodeId &node_id)
{
	m_client = client;
	m_nodeId = node_id;
	load();
}

qfopcua::DataValue AttributesModel::attribute(qfopcua::AttributeId::Enum attr_id) const
{
	//shvLogFuncFrame() << "att_id:" << qfopcua::AttributeId::toString(attr_id);
	qfopcua::DataValue ret;
	if(!m_client.isNull())
		ret = m_client->getAttribute(m_nodeId, attr_id);
	return ret;
}
*/
void AttributesModel::load()
{
	clear();
	/*
	if(m_nodeId.isNull())
		return;
	appendNode(createNode(qfopcua::AttributeId::NodeId));
	appendNode(createNode(qfopcua::AttributeId::NodeClass));
	appendNode(createNode(qfopcua::AttributeId::BrowseName));
	appendNode(createNode(qfopcua::AttributeId::DisplayName));
	appendNode(createNode(qfopcua::AttributeId::Description));
	appendNode(createNode(qfopcua::AttributeId::WriteMask));
	appendNode(createNode(qfopcua::AttributeId::UserWriteMask));
	appendNode(createNode(qfopcua::AttributeId::Value));
	appendNode(createNode(qfopcua::AttributeId::DataType));
	appendNode(createNode(qfopcua::AttributeId::AccessLevel));
	appendNode(createNode(qfopcua::AttributeId::UserAccessLevel));
	*/
}

void AttributesModel::appendNode(AttributeNode *nd, bool load)
{
	QList<QStandardItem*> lst;
	lst << nd;
	lst << new QStandardItem();
	invisibleRootItem()->appendRow(lst);
	if(load)
		nd->load(true);
	//if(nd->attributeId() == qfopcua::AttributeId::UserAccessLevel) {
	//	m_userAccessLevel = nd->value().toInt();
	//}
}
/*
AttributeNode *AttributesModel::createNode(qfopcua::AttributeId::Enum attr_id)
{
	AttributeNode *ret;
	switch(attr_id) {
	case qfopcua::AttributeId::NodeId:
	case qfopcua::AttributeId::DataType:
		ret = new NodeIdAttributeNode(attr_id);
		break;
	case qfopcua::AttributeId::Value:
		ret = new DataValueAttributeNode(attr_id);
		break;
	case qfopcua::AttributeId::AccessLevel:
	case qfopcua::AttributeId::UserAccessLevel:
		ret = new AccessLevelAttributeNode(attr_id);
		break;
	case qfopcua::AttributeId::NodeClass:
		ret = new NodeClassAttributeNode(attr_id);
		break;
	case qfopcua::AttributeId::BrowseName:
		ret = new QualifiedNameAttributeNode(attr_id);
		break;
	case qfopcua::AttributeId::DisplayName:
	case qfopcua::AttributeId::Description:
	case qfopcua::AttributeId::WriteMask:
	case qfopcua::AttributeId::UserWriteMask:
	default:
		ret = new AttributeNode(attr_id);
		break;
	}
	return ret;
}
*/
