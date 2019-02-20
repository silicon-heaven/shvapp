#ifndef DLGSUBSCRIPTION_H
#define DLGSUBSCRIPTION_H

#include "subscriptionswidget.h"
#include <QDialog>


namespace Ui {
class DlgSubscriptions;
}

class DlgSubscriptions : public QDialog
{
	Q_OBJECT

public:
	explicit DlgSubscriptions(QWidget *parent = 0);
	~DlgSubscriptions();

	SubscriptionsWidget *subscriptionsWidget();
private:
	Ui::DlgSubscriptions *ui;
};

#endif // DLGSUBSCRIPTION_H
