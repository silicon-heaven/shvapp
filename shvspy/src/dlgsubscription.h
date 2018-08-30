#ifndef DLGSUBSCRIPTION_H
#define DLGSUBSCRIPTION_H

#include <shv/iotqt/rpc/deviceconnection.h>

#include "attributesmodel/attributesmodel.h"
#include "servertreemodel/servertreemodel.h"
#include "servertreemodel/shvbrokernodeitem.h"
#include "servertreeview.h"

#include <QDialog>

namespace Ui {
class DlgSubscription;
}

class DlgSubscription : public QDialog
{
	Q_OBJECT

public:
	explicit DlgSubscription(QWidget *parent = 0);
	~DlgSubscription();

	void setServerProperties(const QVariantMap &props);
	QVariantMap getServerProperties();
	void setShvServerNode(ShvBrokerNodeItem * shv_broker);
	void setShvPath(std::string path);
private:
	Ui::DlgSubscription *ui;

	ShvBrokerNodeItem *m_shvServerNode;
	ServerTreeModel *m_srvTreeModel;
	QVariantMap m_serverProps;

	QVariantList m_subscriptionList;

private slots:
	void on_subscriptionAddButton_clicked();
	void on_subscriptionDeleteButton_clicked();
};

#endif // DLGSUBSCRIPTION_H
