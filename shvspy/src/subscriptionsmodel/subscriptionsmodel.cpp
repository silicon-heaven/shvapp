#include "subscriptionsmodel.h"

#include "../theapp.h"
#include "../servertreemodel/shvnodeitem.h"
#include "../servertreemodel/shvbrokernodeitem.h"
#include "../servertreemodel/servertreemodel.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/coreqt/log.h>

#include <QBrush>

namespace cp = shv::chainpack;

SubscriptionsModel::SubscriptionsModel(QObject *parent)
	: Super(parent)
{
}

SubscriptionsModel::~SubscriptionsModel()
{
}

int SubscriptionsModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return m_subscriptions.count();
}

int SubscriptionsModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	return Columns::ColCount;
}

Qt::ItemFlags SubscriptionsModel::flags(const QModelIndex &ix) const
{
	if (!ix.isValid()){
		return Qt::NoItemFlags;
	}

	if(ix.column() == Columns::ColMethod || ix.column() == Columns::ColPath){
		return  Super::flags(ix) |= Qt::ItemIsEditable;
	}

	if((ix.column() == Columns::ColEnabled) ||
		(ix.column() == Columns::ColPermanent)){
		return  Super::flags(ix) |= Qt::ItemIsUserCheckable;
	}

	return Super::flags(ix);
}

QVariant SubscriptionsModel::data(const QModelIndex &ix, int role) const
{
	if (m_subscriptions.isEmpty() || ix.row() >= m_subscriptions.count() || ix.row() < 0){
		return QVariant();
	}

	const Subscription &sub = m_subscriptions.at(ix.row());

	if(role == Qt::DisplayRole) {
		switch (ix.column()) {
		case Columns::ColServer:
			return sub.serverName();
		case Columns::ColPath:
			return sub.shvPath();
		case Columns::ColMethod:
			return sub.method();
		}
	}
	else if(role == Qt::CheckStateRole) {
		switch (ix.column()) {
		case Columns::ColPermanent:
			return (sub.isPermanent()) ? Qt::Checked : Qt::Unchecked;
		case Columns::ColEnabled:
			return (sub.isEnabled()) ? Qt::Checked : Qt::Unchecked;
		}
	}
	else if(role == Qt::UserRole) {
		if (ix.column() == Columns::ColServer){
			return sub.brokerId();
		}
	}
	else if(role == Qt::BackgroundRole) {
		if (ix.column() == Columns::ColServer) {
			return QBrush(QColor::fromHsv((sub.brokerId() * 36) % 360, 45, 255));
		}
	}

	return QVariant();
}

bool SubscriptionsModel::setData(const QModelIndex &ix, const QVariant &val, int role)
{
	if (m_subscriptions.isEmpty() || ix.row() >= m_subscriptions.count() || ix.row() < 0){
		return false;
	}

	if (role == Qt::CheckStateRole){
		Subscription &sub = m_subscriptions[ix.row()];
		int col = ix.column();
		bool v = (val == Qt::Checked) ? true : false;

		if (col == Columns::ColPermanent || col == Columns::ColEnabled){
			ShvBrokerNodeItem *nd = TheApp::instance()->serverTreeModel()->brokerById(sub.brokerId());
			if (nd == nullptr){
				return false;
			}

			if (col == Columns::ColPermanent){
				sub.setIsPermanent(v);
			}
			else if (col == Columns::ColEnabled){
				sub.setIsEnabled(v);
				nd->enableSubscription(sub.shvPath().toStdString(), sub.method().toStdString(), v);
			}

			QVariantList new_subs = brokerSubscriptions(sub.brokerId());
			nd->setSubscriptionList(new_subs);

			return true;
		}
	}
	else if (role == Qt::EditRole){
		if (ix.column() == Columns::ColMethod){
			Subscription &sub = m_subscriptions[ix.row()];

			ShvBrokerNodeItem *nd = TheApp::instance()->serverTreeModel()->brokerById(sub.brokerId());
			if (nd == nullptr){
				return false;
			}

			nd->enableSubscription(sub.shvPath().toStdString(), sub.method().toStdString(), false);
			sub.setMethod(val.toString());
			nd->enableSubscription(sub.shvPath().toStdString(), sub.method().toStdString(), true);

			QVariantList new_subs = brokerSubscriptions(sub.brokerId());
			nd->setSubscriptionList(new_subs);
			return true;
		}
		else if (ix.column() == Columns::ColPath){
			Subscription &sub = m_subscriptions[ix.row()];

			ShvBrokerNodeItem *nd = TheApp::instance()->serverTreeModel()->brokerById(sub.brokerId());
			if (nd == nullptr){
				return false;
			}

			nd->enableSubscription(sub.shvPath().toStdString(), sub.method().toStdString(), false);
			sub.setShvPath(val.toString());
			nd->enableSubscription(sub.shvPath().toStdString(), sub.method().toStdString(), true);

			QVariantList new_subs = brokerSubscriptions(sub.brokerId());
			nd->setSubscriptionList(new_subs);
			return true;
		}
	}

	return false;
}

QVariant SubscriptionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;
	if(orientation == Qt::Horizontal) {
		if(role == Qt::DisplayRole) {
			switch (section){
			case Columns::ColServer:
				return tr("Server");
			case Columns::ColPath:
				return tr("Path");
			case Columns::ColMethod:
				 return tr("Method");
			case Columns::ColPermanent:
				return tr("Permanent");
			case Columns::ColEnabled:
				return tr("Enabled");
			default:
				return tr("Unknown");
			}
		}
	}
	return ret;
}

void SubscriptionsModel::reload()
{
	beginResetModel();
	endResetModel();
}

void SubscriptionsModel::onBrokerConnectedChanged(int broker_id, bool is_connected)
{
	ShvBrokerNodeItem *nd = TheApp::instance()->serverTreeModel()->brokerById(broker_id);

	if (nd == nullptr){
		return;
	}

	beginResetModel();
	if (is_connected){
		QMetaEnum met_sub = QMetaEnum::fromType<ShvBrokerNodeItem::SubscriptionItem>();
		QVariant v = nd->serverProperties().value(ShvBrokerNodeItem::SUBSCRIPTIONS);

		if(v.isValid()) {
			QVariantList subs = v.toList();

			for (int i = 0; i < subs.size(); i++) {
				if (subs.at(i).toMap().contains(met_sub.valueToKey(ShvBrokerNodeItem::SubscriptionItem::IsPermanent))){
					SubscriptionsModel::Subscription s(broker_id, QString::fromStdString(nd->nodeId()));
					s.setConfig(subs.at(i).toMap());

					m_subscriptions.append(s);
				}
			}
		}
	}
	else{
		QVariantList subs;
		for (int i = m_subscriptions.size() -1; i >= 0; i--) {
			if (m_subscriptions.at(i).brokerId() == broker_id){
				m_subscriptions.removeAt(i);
			}
		}
	}
	endResetModel();
}

void SubscriptionsModel::addSubscription(Subscription sub)
{
	int sub_ix = subscriptionIndex(sub.brokerId(), sub.shvPath(), sub.method());

	if (sub_ix == -1){
		beginInsertRows(QModelIndex(), m_subscriptions.count(), m_subscriptions.count());
		m_subscriptions.append(sub);
		endInsertRows();
	}
	else{
		m_subscriptions[sub_ix].setIsEnabled(true);
		emit dataChanged(createIndex(sub_ix, Columns::ColEnabled), createIndex(sub_ix, Columns::ColEnabled), QVector<int>() << Qt::CheckStateRole);
	}
}

int SubscriptionsModel::subscriptionIndex(int broker_id, const QString &shv_path, const QString &method)
{
	int sub_ix = -1;
	for (int i = 0; i < m_subscriptions.count(); i++){
		const SubscriptionsModel::Subscription &s = m_subscriptions.at(i);
		if (s.brokerId() == broker_id && s.shvPath() == shv_path && s.method() == method){
			sub_ix = i;
			break;
		}
	}

	return sub_ix;
}

QVariantList SubscriptionsModel::brokerSubscriptions(int broker_id)
{
	QVariantList subscriptions;

	for (int i = 0; i < m_subscriptions.count(); i++){
		const Subscription &s = m_subscriptions.at(i);
		if (s.brokerId() == broker_id && s.isPermanent()){
			subscriptions.append(s.config());
		}
	}
	return  subscriptions;
}

SubscriptionsModel::Subscription::Subscription():
	SubscriptionsModel::Subscription(-1, QString())
{
}

SubscriptionsModel::Subscription::Subscription(int broker_id, const QString &server_name)
{
	m_brokerId = broker_id;
	m_serverName = server_name;
}

void SubscriptionsModel::Subscription::setConfig(const QVariantMap &config)
{
	QMetaEnum meta_sub = QMetaEnum::fromType<ShvBrokerNodeItem::SubscriptionItem>();

	for (int i = 0; i < ShvBrokerNodeItem::SubscriptionItem::Count; i++){
		m_config[i] = config.value(meta_sub.valueToKey(i));
	}
}

QVariantMap SubscriptionsModel::Subscription::config() const
{
	QVariantMap config;
	QMetaEnum meta_sub = QMetaEnum::fromType<ShvBrokerNodeItem::SubscriptionItem>();

	for (int i = 0; i < ShvBrokerNodeItem::SubscriptionItem::Count; i++){
		config[meta_sub.valueToKey(i)] = m_config.value(i);
	}

	return config;
}

int SubscriptionsModel::Subscription::brokerId() const
{
	return m_brokerId;
}

QString SubscriptionsModel::Subscription::serverName() const
{
	return m_serverName;
}

QString SubscriptionsModel::Subscription::shvPath() const
{
	return m_config[ShvBrokerNodeItem::SubscriptionItem::Path].toString();
}

void SubscriptionsModel::Subscription::setShvPath(const QString &shv_path)
{
	m_config[ShvBrokerNodeItem::SubscriptionItem::Path] = shv_path;
}

QString SubscriptionsModel::Subscription::method() const
{
	return m_config[ShvBrokerNodeItem::SubscriptionItem::Method].toString();
}

void SubscriptionsModel::Subscription::setMethod(const QString &method)
{
	m_config[ShvBrokerNodeItem::SubscriptionItem::Method] = method;
}

bool SubscriptionsModel::Subscription::isPermanent() const
{
	return m_config[ShvBrokerNodeItem::SubscriptionItem::IsPermanent].toBool();
}

void SubscriptionsModel::Subscription::setIsPermanent(bool val)
{
	m_config[ShvBrokerNodeItem::SubscriptionItem::IsPermanent] = val;
}

bool SubscriptionsModel::Subscription::isEnabled() const
{
	return m_config[ShvBrokerNodeItem::SubscriptionItem::IsEnabled].toBool();
}

void SubscriptionsModel::Subscription::setIsEnabled(bool val)
{
	m_config[ShvBrokerNodeItem::SubscriptionItem::IsEnabled] = val;
}
