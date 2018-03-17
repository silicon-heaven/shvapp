#include "attributesmodel.h"

#include "../theapp.h"
#include "../servertreemodel/shvnodeitem.h"

#include <shv/chainpack/rpcvalue.h>
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

int AttributesModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	if(m_shvTreeNodeItem.isNull())
		return 0;
	return m_shvTreeNodeItem->methods().count();
}

Qt::ItemFlags AttributesModel::flags(const QModelIndex &ix) const
{
	Qt::ItemFlags ret = Super::flags(ix);
	/*
	bool editable = false;
	if(ix.column() == 1) {
		ValueAttributeNode *nd = dynamic_cast<ValueAttributeNode*>(itemFromIndex(ix.sibling(ix.row(), 0)));
		//shvInfo() << ix.row() << nd << m_userAccessLevel;
		if(nd) {
			//editable = (m_userAccessLevel & qfopcua::AccessLevel::CurrentWrite);
		}
	}
	if(editable)
		ret |= Qt::ItemIsEditable;
	else {
		ret &= ~Qt::ItemIsEditable;
	}
	*/
	return ret;
}

QVariant AttributesModel::data(const QModelIndex &ix, int role) const
{
	if(m_shvTreeNodeItem.isNull())
		return QVariant();
	const QVector<ShvMetaMethod> &mms = m_shvTreeNodeItem->methods();
	if(ix.row() < 0 || ix.row() >= mms.count())
		return QVariant();

	switch (role) {
	case Qt::DisplayRole: {
		if(ix.column() == ColMethodName)
			return QString::fromStdString(mms[ix.row()].name());
		break;
	}
	default:
		break;
	}
	return QVariant();
	/*
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
	*/
}
#if 0
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
#endif
QVariant AttributesModel::headerData(int section, Qt::Orientation o, int role) const
{
	QVariant ret;
	if(o == Qt::Horizontal) {
		if(role == Qt::DisplayRole) {
			if(section == ColMethodName)
				ret = tr("Method");
			else if(section == ColParams)
				ret = tr("Params");
			else if(section == ColResult)
				ret = tr("Result");
		}
	}
	return ret;
}

void AttributesModel::load(ShvNodeItem *nd)
{
	if(!m_shvTreeNodeItem.isNull())
		m_shvTreeNodeItem->disconnect(this);
	m_shvTreeNodeItem = nd;
	if(nd) {
		if(!nd->checkMethodsLoaded())
			connect(nd, &ShvNodeItem::methodsLoaded, this, &AttributesModel::onMethodsLoaded, Qt::UniqueConnection);
	}
	emit layoutChanged();
}

void AttributesModel::onMethodsLoaded()
{
	emit layoutChanged();
}
/*
void AttributesModel::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	if(msg.isResponse()) {
		cp::RpcResponse resp(msg);
		if(resp.requestId() == m_rpcRqId) {
			for(const cp::RpcValue &val : resp.result().toList()) {
				appendRow(QList<QStandardItem*>{
							  new QStandardItem(QString::fromStdString(val.toString())),
							  new QStandardItem("<not called>"),
						  });
			}
		}
	}
}
*/
