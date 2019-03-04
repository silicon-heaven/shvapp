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
	enum TableColumn {tcServer = 0, tcPath, tcMethod, tcPermanent, tcSubscribeAfterConnect, tcEnabled, tcCount};

	explicit SubscriptionsWidget(QWidget *parent = nullptr);
	~SubscriptionsWidget();

	void setShvPath(std::string path);

	void addSubscriptions(const std::string &broker_id, const QVariantList &subscriptions);
	void addSubscription(const std::string &broker_id,const QVariantMap &subscription);
private:
	QString boolToStr(bool val);

	Ui::SubscriptionsWidget *ui;
	QVariantList m_subscriptionList;
};

#endif // SUBSCRIPTIONSWIDGET_H
