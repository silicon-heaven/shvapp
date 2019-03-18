#include "subscriptionsmodel.h"

#include "../theapp.h"
#include "../servertreemodel/shvnodeitem.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/core/utils.h>
#include <shv/core/assert.h>
#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/rpc.h>

#include <QSettings>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QIcon>

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
	int count = 0;

	for (int i = 0; i < m_shvNodeItems.count(); i++){
		QVariant v = m_shvNodeItems.at(i)->serverProperties().value("subscriptions");
		if(v.isValid()) {
			count += v.toMap().size();
		}
	}
	return  count;
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
		return  Super::flags(ix) |= Qt::ItemIsEditable;
	}

	return Super::flags(ix);
}

QVariant SubscriptionsModel::data(const QModelIndex &ix, int role) const
{
	if (m_shvNodeItems.isEmpty())
		return QVariant();

	QVariantMap subscription;

	for (int i = 0; i < m_shvNodeItems.count(); i++){
		QVariant v = m_shvNodeItems.at(i)->serverProperties().value("subscriptions");
		if(v.isValid()) {
			v.toList().at(ix.row);
		}
	}

	if (!subscription.isEmpty()){
		switch (role) {
		case Qt::DisplayRole: {
			switch (ix.column()) {
				ColServer: ;
				ColPath:;
				ColMethod:;
				ColPermanent:;
				ColSubscribeAfterConnect:;
				ColEnabled:;
			}
		}

		}
	}
}


bool SubscriptionsModel::setData(const QModelIndex &ix, const QVariant &val, int role)
{
	return false;
}

QVariant SubscriptionsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	QVariant ret;
	if(orientation == Qt::Horizontal) {
		if(role == Qt::DisplayRole) {
			if(section == Columns::ColServer)
				ret = tr("Server");
			else if(section == Columns::ColPath)
				ret = tr("Path");
			else if(section == Columns::ColMethod)
				ret = tr("Method");
			else if(section == Columns::ColPermanent)
				ret = tr("Permanent");
			else if(section == Columns::ColSubscribeAfterConnect)
				ret = tr("Auto subscribe");
			else if(section == Columns::ColEnabled)
				ret = tr("Enabled");
		}
		else if(role == Qt::ToolTipRole) {
			if(section == Columns::ColSubscribeAfterConnect)
				ret = tr("Subscribe after connect");
		}
	}
	return ret;
}

void SubscriptionsModel::addShvBrokerNodeItem(ShvBrokerNodeItem *nd)
{
	m_shvNodeItems.append(nd);
	connect(nd, &ShvBrokerNodeItem::subscriptionAdded, this, [this, nd](const std::string &path){
		onSubscriptionAdded(nd, path);
	});
}

void SubscriptionsModel::onSubscriptionAdded(ShvBrokerNodeItem *nd, const std::string &path)
{
	shvInfo() << nd->nodeId();
	beginResetModel();
	endResetModel();
}
