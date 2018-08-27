#ifndef DLGSUBSCRIPTION_H
#define DLGSUBSCRIPTION_H

#include <shv/iotqt/rpc/deviceconnection.h>

#include "attributesmodel/attributesmodel.h"
#include "servertreemodel/servertreemodel.h"
#include "servertreemodel/shvbrokernodeitem.h"
#include "servertreeview.h"

#include <QDialog>
#include <QComboBox>

namespace Ui {
class DlgSubscription;
}

class DlgSubscription : public QDialog
{
	Q_OBJECT

public:
	explicit DlgSubscription(QWidget *parent = nullptr);
	DlgSubscription(QWidget *parent = nullptr, ServerTreeModel *model = nullptr, ServerTreeView *view = nullptr);
	~DlgSubscription();

private:
	Ui::DlgSubscription *ui;
	void addSubscription();
	void delSubscription();
	void doneSubscription();
	void showChildrenPopup();
	QList<QPair<QString, QString> > m_subscription;
	ShvNodeItem *m_nd;
	ServerTreeModel *m_model;
	ServerTreeView *m_view;
	ShvBrokerNodeItem *m_snd;

	unsigned m_rqid;

	struct Subscription {
		QString path;
		QString method;
	};
	QList<Subscription> subscriptions;

private slots:
	void lineChanged(unsigned int oldPos, unsigned int newPos);
};

#endif // DLGSUBSCRIPTION_H
