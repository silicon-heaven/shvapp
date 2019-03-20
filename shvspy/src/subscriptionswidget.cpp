#include "subscriptionswidget.h"
#include "ui_subscriptionswidget.h"

#include "theapp.h"
#include "subscriptionsmodel/subscriptionstableitemdelegate.h"

#include <shv/coreqt/log.h>

#include <QSettings>
#include <QStringList>

#include <QDebug>

namespace cp = shv::chainpack;

SubscriptionsWidget::SubscriptionsWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SubscriptionsWidget)
{
	ui->setupUi(this);

	ui->tvSubscriptions->setModel(&m_subscriptionsModel);
	ui->tvSubscriptions->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	ui->tvSubscriptions->verticalHeader()->setDefaultSectionSize(static_cast<int>(fontMetrics().height() * 1.3));
//	ui->tvSubscriptions->setItemDelegate(new SubscriptionsTableItemDelegate(ui->tvSubscriptions));
}

SubscriptionsWidget::~SubscriptionsWidget()
{
	delete ui;
}

void SubscriptionsWidget::onBrokerConnectedChanged(ShvBrokerNodeItem *shv_broker_node_item, bool is_connected)
{
	int broker_id = shv_broker_node_item->brokerId();

	if (is_connected){
		QVariant v = shv_broker_node_item->serverProperties().value("subscriptions");

		if(v.isValid()) {
			QVariantList subs = v.toList();

			for (int i = 0; i < subs.size(); i++) {
				SubscriptionsModel::Subscription s(broker_id, subs.at(i).toMap());
				m_subscriptionsList.append(s);
			}
			m_serverIdToName[broker_id] = QString::fromStdString(shv_broker_node_item->nodeId());
		}
	}
	else{
		QVariantList subs;
		for (int i = m_subscriptionsList.size() -1; i >= 0; i--) {
			if (m_subscriptionsList.at(i).brokerId() == broker_id){
				if (m_subscriptionsList.at(i).isPermanent() == true){
					subs.append(m_subscriptionsList.at(i).data());
				}
				m_subscriptionsList.removeAt(i);
			}
		}

		ShvBrokerNodeItem *nd = TheApp::instance()->serverTreeModel()->brokerById(broker_id);

		if (nd != nullptr){
			nd->setSubscriptionList(subs);
		}
	}

	m_subscriptionsModel.setSubscriptions(&m_subscriptionsList, &m_serverIdToName);
	ui->tvSubscriptions->resizeColumnsToContents();
}

void SubscriptionsWidget::onSubscriptionAdded(int broker_id, const std::string &shv_path, const std::string &method)
{
	qInfo() << "add subscr";

	int sub_ix = subscriptionIndex(broker_id, shv_path, method);
	if (sub_ix == -1){
		QVariantMap data;
		data[ShvBrokerNodeItem::SUBSCR_PATH_KEY] = QString::fromStdString(shv_path);
		data[ShvBrokerNodeItem::SUBSCR_METHOD_KEY] = QString::fromStdString(method);
		data[ShvBrokerNodeItem::SUBSCR_IS_PERMANENT_KEY] = false;
		data[ShvBrokerNodeItem::SUBSCR_IS_SUBSCRIBED_AFTER_CONNECT_KEY] = false;
		data[ShvBrokerNodeItem::SUBSCR_IS_ENABLED_KEY] = true;
		SubscriptionsModel::Subscription sub(broker_id, data);

		m_subscriptionsList.append(sub);
		m_subscriptionsModel.reload();
		ui->tvSubscriptions->resizeColumnsToContents();
	}
}

int SubscriptionsWidget::subscriptionIndex(int broker_id, const std::string &shv_path, const std::string &method)
{
	int sub_ix = -1;
	for (int i = 0; i < m_subscriptionsList.count(); i++){
		const SubscriptionsModel::Subscription s = m_subscriptionsList.at(i);
		if (s.brokerId() == broker_id && s.shvPath() == shv_path && s.method() == method){
			sub_ix = i;
		}
	}

	return sub_ix;
}

