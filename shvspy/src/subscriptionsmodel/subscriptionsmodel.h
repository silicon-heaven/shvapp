#pragma once

#include <shv/chainpack/rpcvalue.h>

#include "../servertreemodel/shvbrokernodeitem.h"

#include <QAbstractTableModel>

namespace shv { namespace chainpack { class RpcMessage; }}

class SubscriptionsModel : public QAbstractTableModel
{
	Q_OBJECT
private:
	typedef QAbstractTableModel Super;
public:
	enum Columns {ColServer = 0, ColPath, ColMethod, ColPermanent, ColSubscribeAfterConnect, ColEnabled, ColCount};

	class Subscription{
	public:
		Subscription();
		Subscription(int broker_id, QVariantMap data);

		const QVariantMap &data() const;
		int brokerId() const;
		std::string shvPath() const;
		std::string method() const;
		bool isPermanent() const;
		void setIsPermanent(bool val);
		bool isSubscribeAfterConnect() const;
		void setIsSubscribeAfterConnect(bool val);
		bool isEnabled() const;
		void setIsEnabled(bool val);
	private:
		int m_brokerId;
		QVariantMap m_data;
	};

public:
	SubscriptionsModel(QObject *parent = nullptr);
	~SubscriptionsModel() Q_DECL_OVERRIDE;
public:
	void setSubscriptions(QVector<SubscriptionsModel::Subscription> *subscriptions, const QMap<int, QString> *server_id_to_name);
	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	Qt::ItemFlags flags(const QModelIndex &ix) const Q_DECL_OVERRIDE;
	QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;
	bool setData(const QModelIndex &ix, const QVariant &val, int role = Qt::EditRole) Q_DECL_OVERRIDE;
	QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;
	void reload();

	Q_SIGNAL void subscriptionEnabled(int broker_id, const std::string &shv_path, const std::string &method);
private:
	QVector<Subscription> *m_subscriptions = nullptr;
	const QMap<int, QString> *m_serverIdToName = nullptr;
};
