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

	ui->subsriptionTableWidget->setRowCount(0);
	ui->subsriptionTableWidget->setColumnCount(2);
	QStringList header;
	header << "Path" << "Method" << "";
	ui->subsriptionTableWidget->setHorizontalHeaderLabels(header);
	ui->subsriptionTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->subsriptionTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->subsriptionTableWidget->verticalHeader()->setVisible(false);

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
		ui->subsriptionTableWidget->insertRow(i);
		ui->subsriptionTableWidget->setRowHeight(i, ui->subsriptionTableWidget->fontInfo().pointSize());
		ui->subsriptionTableWidget->setItem(i, 0, new QTableWidgetItem(subscription.value(QStringLiteral("path")).toString()));
		ui->subsriptionTableWidget->setItem(i, 1, new QTableWidgetItem(subscription.value(QStringLiteral("method")).toString()));
	}
	ui->subsriptionTableWidget->resizeColumnsToContents();

	connect(ui->subsriptionTableWidget, &QTableWidget::cellChanged, this, &DlgSubscriptions::subscriptionEdit);
}

QVariantList DlgSubscriptions::subscriptions()
{
	return m_subscriptionList;
}

void DlgSubscriptions::subscriptionDeleteButton()
{
	QModelIndexList selection = ui->subsriptionTableWidget->selectionModel()->selectedRows();
	if (selection.size() > 0){
		QModelIndex index = selection.at(0);
		ui->subsriptionTableWidget->removeRow(index.row());
		m_subscriptionList.removeAt(index.row());
	}
}

void DlgSubscriptions::subscriptionAddButton()
{
	ui->subsriptionTableWidget->insertRow(0);
	ui->subsriptionTableWidget->setRowHeight(0, ui->subsriptionTableWidget->fontInfo().pointSize());
	ui->subsriptionTableWidget->setItem(0, 0, new QTableWidgetItem(ui->subscriptionPathLineEdit->text()));
	ui->subsriptionTableWidget->setItem(0, 1, new QTableWidgetItem(ui->subscriptionMethodLineEdit->text()));

	QVariantMap subscription;
	subscription["path"] = ui->subscriptionPathLineEdit->text();
	subscription["method"] = ui->subscriptionMethodLineEdit->text();

	shvInfo() << m_subscriptionList.size();
	m_subscriptionList.insert(0, subscription);
	shvInfo() << m_subscriptionList.size();

	shvInfo() << "Add subscriptions" << subscription.value(QStringLiteral("path")).toString() << subscription.value(QStringLiteral("method")).toString();
	ui->subsriptionTableWidget->resizeColumnsToContents();
}

void DlgSubscriptions::subscriptionEdit(int row, int column)
{
	QVariantMap subscription;
	subscription["path"] = ui->subsriptionTableWidget->item(row, 0)->text();
	subscription["method"] = ui->subsriptionTableWidget->item(row, 1)->text();

	m_subscriptionList.removeAt(row);
	m_subscriptionList.insert(row, subscription);
}
