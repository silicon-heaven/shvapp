#pragma once

#include <shv/chainpack/rpcvalue.h>

#include "../servertreemodel/shvbrokernodeitem.h"

#include <QAbstractTableModel>
#include <QPointer>

namespace shv { namespace chainpack { class RpcMessage; }}

class SubscriptionsModel : public QAbstractTableModel
{
	Q_OBJECT
private:
	typedef QAbstractTableModel Super;
public:
	enum Columns {ColServer = 0, ColPath, ColMethod, ColPermanent, ColSubscribeAfterConnect, ColEnabled, ColCount};

public:
	SubscriptionsModel(QObject *parent = nullptr);
	~SubscriptionsModel() Q_DECL_OVERRIDE;
public:
	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	Qt::ItemFlags flags(const QModelIndex &ix) const Q_DECL_OVERRIDE;
	QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;
	bool setData(const QModelIndex &ix, const QVariant &val, int role = Qt::EditRole) Q_DECL_OVERRIDE;
	QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;

	void addShvBrokerNodeItem(ShvBrokerNodeItem *nd);

	QString path() const;

private:
	void onSubscriptionAdded(ShvBrokerNodeItem *nd, const std::string &path);

	QVariantList m_subscriptionList;
};
