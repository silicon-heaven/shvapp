#include "subscriptionsmodel.h"

#include "../theapp.h"
#include "../servertreemodel/shvnodeitem.h"
#include "../servertreemodel/shvbrokernodeitem.h"
#include "../servertreemodel/servertreemodel.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/core/utils.h>
#include <shv/core/assert.h>
#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/rpc.h>

#include <QSettings>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QIcon>
#include <QBrush>
#include <QDebug>

namespace cp = shv::chainpack;

SubscriptionsModel::SubscriptionsModel(QObject *parent)
	: Super(parent)
{
}

SubscriptionsModel::~SubscriptionsModel()
{
}

const QString SubscriptionsModel::boolToStr(bool val)
{
	return (val == true) ? tr("yes") : tr("no");
}

void SubscriptionsModel::setSubscriptions(QVector<SubscriptionsModel::Subscription> *subscriptions, const QMap<int, QString> *server_id_to_name)
{
	beginResetModel();
	m_subscriptions = subscriptions;
	m_serverIdToName = server_id_to_name;
	endResetModel();
}

int SubscriptionsModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return (m_subscriptions != nullptr) ? m_subscriptions->count() : 0;
}

int SubscriptionsModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return 0;

	return Columns::ColCount;
}

Qt::ItemFlags SubscriptionsModel::flags(const QModelIndex &ix) const
{
	if (!ix.isValid())
		return Qt::NoItemFlags;

	if((ix.column() == Columns::ColEnabled) ||
		(ix.column() == Columns::ColPermanent) ||
		(ix.column() == Columns::ColSubscribeAfterConnect)){
		return  Super::flags(ix) |= Qt::ItemIsUserCheckable;
	}

	return Super::flags(ix);
}

QVariant SubscriptionsModel::data(const QModelIndex &ix, int role) const
{
	if ((m_subscriptions == nullptr) || (ix.row() >= m_subscriptions->count())){
		return QVariant();
	}

	const Subscription &sub = m_subscriptions->at(ix.row());

	if(role == Qt::DisplayRole) {
		switch (ix.column()) {
		case Columns::ColServer:
			return (m_serverIdToName != nullptr) ? m_serverIdToName->value(sub.brokerId()) : tr("Unknown");
		case Columns::ColPath:
			return QString::fromStdString(sub.shvPath());
		case Columns::ColMethod:
			return QString::fromStdString(sub.method());
		}
	}
	else if(role == Qt::CheckStateRole) {
		switch (ix.column()) {
		case Columns::ColPermanent:
			return (sub.isPermanent()) ? Qt::Checked : Qt::Unchecked;
		case Columns::ColSubscribeAfterConnect:
			return (sub.isSubscribeAfterConnect()) ? Qt::Checked : Qt::Unchecked;
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
			return QBrush(QColor::fromHsv(sub.brokerId() * 36, 45, 255));
		}
	}

	return QVariant();
}

bool SubscriptionsModel::setData(const QModelIndex &ix, const QVariant &val, int role)
{
	if (role == Qt::CheckStateRole){
		Subscription sub = m_subscriptions->value(ix.row());
		int col = ix.column();
		bool v = (val == Qt::Checked) ? true : false;

		if (col == Columns::ColPermanent || col == Columns::ColSubscribeAfterConnect || col == Columns::ColEnabled){
			if (col == Columns::ColPermanent){
				sub.setIsPermanent(v);
			}
			else if (col == Columns::ColSubscribeAfterConnect){
				sub.setIsSubscribeAfterConnect(v);
			}
			else if (col == Columns::ColEnabled){
				sub.setIsEnabled(v);
				ShvBrokerNodeItem *nd = TheApp::instance()->serverTreeModel()->brokerById(sub.brokerId());

				if (nd != nullptr){
					nd->enableSubscription(sub.shvPath(), sub.method(), v);
				}
			}
			m_subscriptions->replace(ix.row(), sub);
			emit dataChanged(createIndex(ix.row(), 0), createIndex(ix.row(), Columns::ColCount-1));
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
			case Columns::ColSubscribeAfterConnect:
				return tr("Auto subscribe");
			case Columns::ColEnabled:
				return tr("Enabled");
			default:
				return tr("Unknown");
			}
		}
		else if(role == Qt::ToolTipRole) {
			if(section == Columns::ColSubscribeAfterConnect)
				ret = tr("Subscribe after connect");
		}
	}
	return ret;
}

void SubscriptionsModel::reload()
{
	beginResetModel();
	endResetModel();
}

SubscriptionsModel::Subscription::Subscription():
	SubscriptionsModel::Subscription(-1, QVariantMap())
{

}

SubscriptionsModel::Subscription::Subscription(int broker_id, QVariantMap data)
{
	m_brokerId = broker_id;
	m_data = data;
}

const QVariantMap &SubscriptionsModel::Subscription::data() const
{
	return m_data;
}

int SubscriptionsModel::Subscription::brokerId() const
{
	return m_brokerId;
}

std::string SubscriptionsModel::Subscription::shvPath() const
{
	return m_data.value(ShvBrokerNodeItem::SUBSCR_PATH_KEY).toString().toStdString();
}

std::string SubscriptionsModel::Subscription::method() const
{
	return m_data.value(ShvBrokerNodeItem::SUBSCR_METHOD_KEY).toString().toStdString();
}

bool SubscriptionsModel::Subscription::isPermanent() const
{
	return m_data.value(ShvBrokerNodeItem::SUBSCR_IS_PERMANENT_KEY).toBool();
}

void SubscriptionsModel::Subscription::setIsPermanent(bool val)
{
	m_data[ShvBrokerNodeItem::SUBSCR_IS_PERMANENT_KEY] = val;
}

bool SubscriptionsModel::Subscription::isSubscribeAfterConnect() const
{
	return m_data.value(ShvBrokerNodeItem::SUBSCR_IS_SUBSCRIBED_AFTER_CONNECT_KEY).toBool();
}

void SubscriptionsModel::Subscription::setIsSubscribeAfterConnect(bool val)
{
	m_data[ShvBrokerNodeItem::SUBSCR_IS_SUBSCRIBED_AFTER_CONNECT_KEY] = val;
}

bool SubscriptionsModel::Subscription::isEnabled() const
{
	return m_data.value(ShvBrokerNodeItem::SUBSCR_IS_ENABLED_KEY).toBool();
}

void SubscriptionsModel::Subscription::setIsEnabled(bool val)
{
	m_data[ShvBrokerNodeItem::SUBSCR_IS_ENABLED_KEY] = val;
}
