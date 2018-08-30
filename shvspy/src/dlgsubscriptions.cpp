#include "dlgsubscriptions.h"
#include "ui_dlgsubscription.h"

#include <shv/coreqt/log.h>

#include <QSettings>
#include <QStringList>

namespace cp = shv::chainpack;

DlgSubscriptions::DlgSubscriptions(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::DlgSubscriptions)
{
	ui->setupUi(this);

	ui->subsriptionsTableWidget->setRowCount(0);
	ui->subsriptionsTableWidget->setColumnCount(2);
	QStringList header;
	header << "Path" << "Method" << "";
	ui->subsriptionsTableWidget->setHorizontalHeaderLabels(header);
	ui->subsriptionsTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->subsriptionsTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->subsriptionsTableWidget->verticalHeader()->setVisible(false);

	connect(ui->addSubsriptionButton,	&QToolButton::clicked, this, &DlgSubscriptions::subscriptionAddButton);
	connect(ui->deleteSubscriptionButton, &QToolButton::clicked, this, &DlgSubscriptions::subscriptionDeleteButton);
}

DlgSubscriptions::~DlgSubscriptions()
{
	delete ui;
}

void DlgSubscriptions::setShvPath(std::string path)
{
	ui->subscriptionPathLineEdit->setText(QString::fromStdString(path));
}

void DlgSubscriptions::setSubscriptionsList(const QVariantList &subs)
{
	m_subscriptionList = subs;

	for (int i = 0; i < m_subscriptionList.size(); i++) {
		QVariantMap subscription = m_subscriptionList.at(i).toMap();

		shvInfo() << "Loaded subscription" << subscription.value(QStringLiteral("path")).toString() << subscription.value(QStringLiteral("method")).toString();
		ui->subsriptionsTableWidget->insertRow(i);
		ui->subsriptionsTableWidget->setRowHeight(i, ui->subsriptionsTableWidget->fontInfo().pointSize());
		ui->subsriptionsTableWidget->setItem(i, 0, new QTableWidgetItem(subscription.value(QStringLiteral("path")).toString()));
		ui->subsriptionsTableWidget->setItem(i, 1, new QTableWidgetItem(subscription.value(QStringLiteral("method")).toString()));
	}
	ui->subsriptionsTableWidget->resizeColumnsToContents();
}

QVariantList DlgSubscriptions::subscriptions()
{
	return m_subscriptionList;
}

void DlgSubscriptions::subscriptionDeleteButton()
{
	QModelIndexList selection = ui->subsriptionsTableWidget->selectionModel()->selectedRows();
	if (selection.size() > 0){
		QModelIndex index = selection.at(0);
		ui->subsriptionsTableWidget->removeRow(index.row());
	}
}

void DlgSubscriptions::subscriptionAddButton()
{
	ui->subsriptionsTableWidget->insertRow(0);
	ui->subsriptionsTableWidget->setRowHeight(0, ui->subsriptionsTableWidget->fontInfo().pointSize());
	ui->subsriptionsTableWidget->setItem(0, 0, new QTableWidgetItem(ui->subscriptionPathLineEdit->text()));
	ui->subsriptionsTableWidget->setItem(0, 1, new QTableWidgetItem(ui->subscriptionMethodLineEdit->text()));

	shvInfo() << "Add subscriptions" << ui->subscriptionPathLineEdit->text() << ui->subscriptionMethodLineEdit->text();
	ui->subsriptionsTableWidget->resizeColumnsToContents();
}

void DlgSubscriptions::on_buttonBox_accepted(){
	m_subscriptionList.clear();
	for (int i = 0; i < ui->subsriptionsTableWidget->rowCount(); i++){
		QVariantMap subscription;
		subscription["path"] = ui->subsriptionsTableWidget->item(i, 0)->text();
		subscription["method"] = ui->subsriptionsTableWidget->item(i, 1)->text();
		m_subscriptionList.insert(i, subscription);
	}
}
