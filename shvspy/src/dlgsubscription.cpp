#include "dlgsubscription.h"
#include "ui_dlgsubscription.h"

#include <QSettings>
#include <QStringList>

#include <shv/coreqt/log.h>

namespace cp = shv::chainpack;

DlgSubscription::DlgSubscription(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::DlgSubscription)
{
	ui->setupUi(this);

	m_shvServerNode = nullptr;

	ui->subsriptionTableWidget->setRowCount(0);
	ui->subsriptionTableWidget->setColumnCount(2);
	QStringList header;
	header << "Path" << "Method" << "";
	ui->subsriptionTableWidget->setHorizontalHeaderLabels(header);
	ui->subsriptionTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->subsriptionTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->subsriptionTableWidget->verticalHeader()->setVisible(false);

	connect(ui->addSubsriptionButton,	&QToolButton::clicked, this, &DlgSubscription::on_subscriptionAddButton_clicked);
	connect(ui->deleteSubscriptionButton, &QToolButton::clicked, this, &DlgSubscription::on_subscriptionDeleteButton_clicked);
}


DlgSubscription::~DlgSubscription()
{
	delete ui;
}

void DlgSubscription::setShvServerNode(ShvBrokerNodeItem * shv_broker)
{
	m_shvServerNode = shv_broker;
}

void DlgSubscription::setShvPath(std::string path)
{
	ui->subscriptionPathLineEdit->setText(QString::fromStdString(path));
}

void DlgSubscription::setServerProperties(const QVariantMap &props)
{
	m_serverProps = props;

	shvInfo() << m_serverProps["subscriptions"].toList().size();
	QVariant v = m_serverProps.value("subscriptions");
	if(v.isValid()) {
		m_subscriptionList = v.toList();
		for (int i = 0; i < m_subscriptionList.size(); i++) {
			QVariantMap subscription = m_subscriptionList.at(i).toMap();

			shvInfo() << "Loaded subscription" << subscription["path"].toString() << subscription["method"].toString();
			ui->subsriptionTableWidget->insertRow(i);
			ui->subsriptionTableWidget->setRowHeight(i, ui->subsriptionTableWidget->fontInfo().pointSize());
			ui->subsriptionTableWidget->setItem(i, 0, new QTableWidgetItem(subscription["path"].toString()));
			ui->subsriptionTableWidget->setItem(i, 1, new QTableWidgetItem(subscription["method"].toString()));
		}
	}
	ui->subsriptionTableWidget->resizeColumnsToContents();
}

QVariantMap DlgSubscription::getServerProperties()
{
	return m_serverProps;
}

void DlgSubscription::on_subscriptionDeleteButton_clicked()
{
	QModelIndexList selection = ui->subsriptionTableWidget->selectionModel()->selectedRows();
	if (selection.size() > 0){
		QModelIndex index = selection.at(0);
		ui->subsriptionTableWidget->removeRow(index.row());
		m_subscriptionList.removeAt(index.row());
	}
}

void DlgSubscription::on_subscriptionAddButton_clicked()
{
	if (m_shvServerNode)
		m_shvServerNode->callCreateSubscription(ui->subscriptionPathLineEdit->text().toStdString(), ui->subscriptionMethodLineEdit->text().toStdString());
	ui->subsriptionTableWidget->insertRow(0);
	ui->subsriptionTableWidget->setRowHeight(0, ui->subsriptionTableWidget->fontInfo().pointSize());
	ui->subsriptionTableWidget->setItem(0, 0, new QTableWidgetItem(ui->subscriptionPathLineEdit->text()));
	ui->subsriptionTableWidget->setItem(0, 1, new QTableWidgetItem(ui->subscriptionMethodLineEdit->text()));

	QVariantMap subscription;
	subscription["path"] = ui->subscriptionPathLineEdit->text();
	subscription["method"] = ui->subscriptionMethodLineEdit->text();

	if (m_shvServerNode) m_serverProps = m_shvServerNode->serverProperties();
	m_subscriptionList = m_serverProps.value("subscriptions").toList();
	shvInfo() << m_subscriptionList.size();
	m_subscriptionList.insert(0, subscription);
	shvInfo() << m_subscriptionList.size();

	m_serverProps["subscriptions"] = m_subscriptionList;

	shvInfo() << "Add subscriptions" << subscription["path"].toString() << subscription["method"].toString();
	ui->subsriptionTableWidget->resizeColumnsToContents();
}
