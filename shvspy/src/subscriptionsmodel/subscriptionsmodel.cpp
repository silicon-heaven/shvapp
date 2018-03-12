#include "subscriptionsmodel.h"

#if 0

#include <qfopcua/client.h>

#include <qf/core/log.h>

#include <QMetaObject>

SubscriptionsModel::SubscriptionsModel(QObject *parent)
	: Super(parent)
{

}

int SubscriptionsModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return m_items.count();
}

int SubscriptionsModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return ColLast;
}

QVariant SubscriptionsModel::data(const QModelIndex &ix, int role) const
{
	if(!ix.isValid())
		return QVariant();
	if(role == Qt::DisplayRole) {
		const Item &it = m_items[ix.row()];
		int col = ix.column();
		switch(col) {
		case ColServer: {
			qfopcua::Subscription *ss = it.subscription;
			qfopcua::Client *cli = ss->client();
			return cli->serverName();
		}
		case ColNodeId: {
			return it.nodeId.toString();
		}
		case ColName: {
			return it.qualifiedName.toString();
		}
		case ColValue: {
			QVariant val = it.value;
			QDateTime dt = val.toDateTime();
			if(dt.isValid())
				return dt.toString(Qt::ISODate);
			if(val.userType() == qMetaTypeId<QChar>())
				return QString("%1 %2").arg(val.toChar()).arg(val.toChar().unicode());
			if(val.userType() == qMetaTypeId<char>())
				return QString("'%1' %2").arg(qvariant_cast<char>(val)).arg((int)qvariant_cast<char>(val));
			if(val.userType() == qMetaTypeId<uchar>())
				return QString("'%1' %2").arg((char)qvariant_cast<uchar>(val)).arg((int)qvariant_cast<uchar>(val));
			//shvInfo() << val << val.typeName();
			return it.value;
		}
		}
	}
	return QVariant();
}

QVariant SubscriptionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		switch(section) {
		case ColServer:
			return tr("Server");
		case ColNodeId:
			return tr("Node id");
		case ColName:
			return tr("Node name");
		case ColValue:
			return tr("Value");
		}
	}
	return Super::headerData(section, orientation, role);
}

bool SubscriptionsModel::removeRows(int row, int count, const QModelIndex &parent)
{
	shvLogFuncFrame() << "row:" << row << "count:" << count;
	int row2 = row + count - 1;
	if(row2 > row) {
		beginRemoveRows(parent, row, row2);
		for (int i = row; i <= row2; ++i) {
			m_items.removeAt(i);
		}
		endRemoveRows();
	}
	return true;
}

void SubscriptionsModel::addItem(qfopcua::Subscription *subscr, qfopcua::Subscription::MonitoredItemId monitored_item_id)
{
	shvLogFuncFrame();
	beginInsertRows(QModelIndex(), rowCount(), rowCount());
	Item it;
	it.subscription = subscr;
	it.monitoredItemId = monitored_item_id;
	m_items.append(it);
	QMetaObject::Connection c = connect(subscr, &qfopcua::Subscription::dataChanged, this, &SubscriptionsModel::onDataChanged, Qt::UniqueConnection);
	qfDebug() << "connection success:" << (bool)c;
	endInsertRows();
}

void SubscriptionsModel::onDataChanged(const qfopcua::DataValue &data_value, int att_id, const qfopcua::NodeId &node_id, qfopcua::Subscription::MonitoredItemId monitored_item_id, qfopcua::Subscription::Id subscription_id)
{
	shvLogFuncFrame() << "DATA val:" << data_value.toString() << "attr:" << qfopcua::AttributeId::toString(qfopcua::AttributeId::fromInt(att_id)) << "node:" << node_id.toString() << "monitored item id:" << monitored_item_id << "subscriber id:" << subscription_id;
	for (int i = 0; i < m_items.count(); ++i) {
		Item &it = m_items[i];
		if(it.monitoredItemId == monitored_item_id && it.subscription->id() == subscription_id) {
			it.value = data_value.value();
			it.nodeId = node_id;
			it.attributeId = qfopcua::AttributeId::fromInt(att_id);
			if(!node_id.isNull() && it.qualifiedName.isNull()) {
				qfopcua::Client *cli = it.subscription->client();
				it.qualifiedName = cli->getQualifiedName(node_id);
			}
			QModelIndex ix1 = index(i, 0);
			QModelIndex ix2 = index(i, ColLast - 1);
			emit dataChanged(ix1, ix2);
			break;
		}
	}
}

#endif
