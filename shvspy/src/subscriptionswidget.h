#ifndef SUBSCRIPTIONSWIDGET_H
#define SUBSCRIPTIONSWIDGET_H

#include <shv/iotqt/rpc/deviceconnection.h>

#include "attributesmodel/attributesmodel.h"
#include "servertreemodel/servertreemodel.h"
#include "servertreemodel/shvbrokernodeitem.h"
#include "servertreeview.h"

#include <QWidget>
#include <QTableWidgetItem>

namespace Ui {
class SubscriptionsWidget;
}

class SubscriptionsWidget : public QWidget
{
	Q_OBJECT

public:
	explicit SubscriptionsWidget(QWidget *parent = 0);
	~SubscriptionsWidget();

	void setSubscriptionsList(const QVariantList &props);
	QVariantList subscriptionsList();
	void setShvPath(std::string path);

private slots:
	void onAddSubscription();
	void onDeleteSubscription();
	void on_buttonBox_accepted();

private:
	Ui::SubscriptionsWidget *ui;
	QVariantList m_subscriptionList;
};

#endif // SUBSCRIPTIONSWIDGET_H
