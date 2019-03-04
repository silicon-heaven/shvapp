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
//	void load(ShvNodeItem *nd);

	QString path() const;
	QString method(int row) const;

	Q_SIGNAL void reloaded();
	Q_SIGNAL void methodCallResultChanged(int method_ix);
private:
	void onSubscriptionAdded(ShvBrokerNodeItem *nd, const std::string &path);
	void onMethodsLoaded();
	void onRpcMethodCallFinished(int method_ix);
//	const ShvMetaMethod *metaMethodAt(unsigned method_ix);
	void loadRow(unsigned method_ix);
	void loadRows();
	void emitRowChanged(int row_ix);
	void callGetters();
private:
	QVector<QPointer<ShvBrokerNodeItem>> m_shvNodeItems;
	using RowVals = shv::chainpack::RpcValue::List;
	std::vector<RowVals> m_rows;
};
