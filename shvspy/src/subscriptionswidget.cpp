#include "subscriptionswidget.h"
#include "ui_subscriptionswidget.h"

#include "subscriptionsmodel/subscriptionstableitemdelegate.h"

#include <shv/coreqt/log.h>

#include <QSettings>
#include <QStringList>

namespace cp = shv::chainpack;

SubscriptionsWidget::SubscriptionsWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SubscriptionsWidget)
{
	ui->setupUi(this);

	ui->tvSubscriptions->setModel(&m_subscriptionsModel);
	ui->tvSubscriptions->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	ui->tvSubscriptions->verticalHeader()->setDefaultSectionSize(static_cast<int>(fontMetrics().height() * 1.3));
	ui->tvSubscriptions->setItemDelegate(new SubscriptionsTableItemDelegate(ui->tvSubscriptions));
}

SubscriptionsWidget::~SubscriptionsWidget()
{
	delete ui;
}

void SubscriptionsWidget::subscriptionsCreated(ShvBrokerNodeItem *shv_broker_node_item)
{
	m_subscriptionsModel.addShvBrokerNodeItem(shv_broker_node_item);
}
