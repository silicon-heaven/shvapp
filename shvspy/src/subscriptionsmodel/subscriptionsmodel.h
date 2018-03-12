#ifndef SUBSCRIPTIONSMODEL_H
#define SUBSCRIPTIONSMODEL_H

#if 0

#include <qfopcua/subscription.h>
#include <qfopcua/nodeid.h>
#include <qfopcua/qualifiedname.h>

#include <QAbstractTableModel>

namespace qfopcua {
class Subscription;
}

class SubscriptionsModel : public QAbstractTableModel
{
	//Q_OBJECT
	typedef QAbstractTableModel Super;
public:
	SubscriptionsModel(QObject *parent = nullptr);
private:
	enum Columns {ColServer = 0, ColNodeId, ColName, ColValue, ColLast};
public:
	int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
	int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data(const QModelIndex &ix, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
	//bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;
	bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) Q_DECL_OVERRIDE;
	void clear() {removeRows(0, rowCount());}
	void addItem(qfopcua::Subscription *subscr, qfopcua::Subscription::MonitoredItemId monitoredItemId);
private:
	Q_SLOT void onDataChanged(const qfopcua::DataValue &data_value, int att_id, const qfopcua::NodeId &node_id, qfopcua::Subscription::MonitoredItemId monitored_item_id, qfopcua::Subscription::Id subscription_id);
private:
	struct Item
	{
		qfopcua::Subscription *subscription = nullptr;
		qfopcua::Subscription::MonitoredItemId monitoredItemId = 0;
		qfopcua::NodeId nodeId;
		qfopcua::QualifiedName qualifiedName;
		qfopcua::AttributeId::Enum attributeId = qfopcua::AttributeId::Unknown;
		QVariant value;
	};

	QList<Item> m_items;
};
#endif
#endif // SUBSCRIPTIONSMODEL_H
