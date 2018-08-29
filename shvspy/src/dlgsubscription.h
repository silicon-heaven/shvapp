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
	DlgSubscription(QWidget *parent = nullptr, ServerTreeModel *model = nullptr, ServerTreeView *view = nullptr);
	~DlgSubscription();

private:
	Ui::DlgSubscription *ui;

	ShvBrokerNodeItem *m_shvServerNode;
	ServerTreeModel *m_srvTreeModel;
	QVariantMap m_ServerProps;

	QList<QVariant> m_subscriptionList;

private slots:
	void onDialogOkButtonClicked();
	void onSubscriptionAddButtonClicked();
	void onSubscriptionDeleteButtonClicked();
};

#endif // DLGSUBSCRIPTION_H
