#ifndef DLGSUBSCRIPTION_H
#define DLGSUBSCRIPTION_H

#include <shv/iotqt/rpc/deviceconnection.h>

#include "attributesmodel/attributesmodel.h"
#include "servertreemodel/servertreemodel.h"
#include "servertreemodel/shvbrokernodeitem.h"
#include "servertreeview.h"

#include <QDialog>
#include <QTableWidgetItem>

namespace Ui {
class DlgSubscriptions;
}

class DlgSubscriptions : public QDialog
{
	Q_OBJECT

public:
	explicit DlgSubscriptions(QWidget *parent = 0);
	~DlgSubscriptions();

	void setSubscriptionsList(const QVariantList &props);
	QVariantList subscriptions();
	void setShvPath(std::string path);

private slots:
	void subscriptionAddButton();
	void subscriptionDeleteButton();
	void on_buttonBox_accepted();

private:
	Ui::DlgSubscriptions *ui;
	QVariantList m_subscriptionList;
};

#endif // DLGSUBSCRIPTION_H
