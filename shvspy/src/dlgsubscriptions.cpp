#include "dlgsubscriptions.h"
#include "ui_dlgsubscriptions.h"

#include <shv/coreqt/log.h>

#include <QSettings>
#include <QStringList>

namespace cp = shv::chainpack;

DlgSubscriptions::DlgSubscriptions(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::DlgSubscriptions)
{
	ui->setupUi(this);
}

DlgSubscriptions::~DlgSubscriptions()
{
	delete ui;
}

SubscriptionsWidget *DlgSubscriptions::subscriptionsWidget()
{
	return ui->subscriptionsWidget;
}
