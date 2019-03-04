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

	void subscriptionsCreated(ShvBrokerNodeItem *shv_broker_node_item);
private:
	Ui::SubscriptionsWidget *ui;
	SubscriptionsModel m_subscriptionsModel;
};

#endif // SUBSCRIPTIONSWIDGET_H
