#include "subscriptionswidget.h"
#include "ui_subscriptionswidget.h"
#include "subscriptionstableitemdelegate.h"

#include <shv/coreqt/log.h>

#include <QSettings>
#include <QStringList>

namespace cp = shv::chainpack;

SubscriptionsWidget::SubscriptionsWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::SubscriptionsWidget)
{
	ui->setupUi(this);

	ui->subsriptionsTableWidget->setRowCount(0);
	ui->subsriptionsTableWidget->setColumnCount(TableColumn::tcCount);
	QStringList header;
	header << "Server" << "Path" << "Method" << "Permanent" << "Auto subscribe" << "Enabled";

	ui->subsriptionsTableWidget->verticalHeader()->setDefaultSectionSize(static_cast<int>(fontMetrics().lineSpacing() * 1.3));
	ui->subsriptionsTableWidget->setHorizontalHeaderLabels(header);
	ui->subsriptionsTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->subsriptionsTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->subsriptionsTableWidget->verticalHeader()->setVisible(false);
	ui->subsriptionsTableWidget->resizeColumnsToContents();

	ui->subsriptionsTableWidget->setItemDelegate(new SubscriptionsTableItemDelegate(ui->subsriptionsTableWidget));
}

SubscriptionsWidget::~SubscriptionsWidget()
{
	delete ui;
}

void SubscriptionsWidget::addSubscriptions(const std::string &broker_id, const QVariantList &subscriptions)
{
	m_subscriptionList = subscriptions;

	for (int i = 0; i < m_subscriptionList.size(); i++) {
		addSubscription(broker_id, m_subscriptionList.at(i).toMap());
	}

	ui->subsriptionsTableWidget->resizeColumnsToContents();
}

void SubscriptionsWidget::addSubscription(const std::string &broker_id, const QVariantMap &subscription)
{
	int row_count = ui->subsriptionsTableWidget->rowCount();
	ui->subsriptionsTableWidget->insertRow(row_count);

	QTableWidgetItem *item;
	item = new QTableWidgetItem(QString::fromStdString(broker_id));
	item->setFlags(item->flags() & ~Qt::ItemIsEditable);
	ui->subsriptionsTableWidget->setItem(row_count, TableColumn::tcServer, item);

	item = new QTableWidgetItem(subscription.value(QStringLiteral("path")).toString());
	item->setFlags(item->flags() & ~Qt::ItemIsEditable);
	ui->subsriptionsTableWidget->setItem(row_count, TableColumn::tcPath, item);

	item = new QTableWidgetItem(subscription.value(QStringLiteral("method")).toString());
	item->setFlags(item->flags() & ~Qt::ItemIsEditable);
	ui->subsriptionsTableWidget->setItem(row_count, TableColumn::tcMethod, item);

	bool val = subscription.value(QStringLiteral("isPermanent")).toBool();
	item = new QTableWidgetItem(boolToStr(val));
	item->setData(Qt::UserRole, val);
	ui->subsriptionsTableWidget->setItem(row_count, TableColumn::tcPermanent, item);

	val = subscription.value(QStringLiteral("isSubscribedAfterConnect")).toBool();
	item = new QTableWidgetItem(boolToStr(val));
	item->setData(Qt::UserRole, val);
	ui->subsriptionsTableWidget->setItem(row_count, TableColumn::tcSubscribeAfterConnect, item);

	val = subscription.value(QStringLiteral("isEnabled")).toBool();
	item = new QTableWidgetItem(boolToStr(val));
	item->setData(Qt::UserRole, val);
	ui->subsriptionsTableWidget->setItem(row_count, TableColumn::tcEnabled, item);
}

QString SubscriptionsWidget::boolToStr(bool val)
{
	return (val) ? tr("yes") : tr("no");
}
