#ifndef SUBSCRIPTIONSWIDGET_H
#define SUBSCRIPTIONSWIDGET_H

#include <shv/iotqt/rpc/deviceconnection.h>

#include "attributesmodel/attributesmodel.h"
#include "servertreemodel/servertreemodel.h"
#include "servertreemodel/shvbrokernodeitem.h"
#include "subscriptionsmodel/subscriptionsmodel.h"

#include <QWidget>
#include <QTableWidgetItem>

namespace Ui {
class SubscriptionsWidget;
}

class SubscriptionsWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SubscriptionsWidget(QWidget *parent = nullptr);
	~SubscriptionsWidget();

	void onBrokerConnectedChanged(ShvBrokerNodeItem *shv_broker_node_item, bool is_connected);
	void onSubscriptionAdded(int broker_id, const std::string &shv_path, const std::string &method);
	void onSubscriptionRemoved(int broker_id, const std::string &shv_path, const std::string &method);
private:
	int subscriptionIndex(int broker_id, const std::string &shv_path, const std::string &method);

	Ui::SubscriptionsWidget *ui;
	SubscriptionsModel m_subscriptionsModel;
	QVector<SubscriptionsModel::Subscription> m_subscriptionsList;
	QMap<int, QString> m_serverIdToName;
};

#endif // SUBSCRIPTIONSWIDGET_H
