#include "dlgsubscription.h"
#include "ui_dlgsubscription.h"

#include <QSettings>
#include <QStringList>

#include <shv/coreqt/log.h>

namespace cp = shv::chainpack;

DlgSubscription::DlgSubscription(QWidget *parent, ServerTreeModel *model, ServerTreeView *view)
	: QDialog(parent)
	, ui(new Ui::DlgSubscription)
	, m_srvTreeModel(model)
{
	ui->setupUi(this);

	ShvNodeItem *shv_nd_item;
	shv_nd_item = model->itemFromIndex(view->currentIndex());

	m_shvServerNode = shv_nd_item->serverNode();

	ui->subscriptionPathLineEdit->setText(shv_nd_item->shvPath().data());

	ui->subsriptionTableWidget->setRowCount(0);
	ui->subsriptionTableWidget->setColumnCount(2);
	QStringList header;
	header << "Path" << "Method" << "";
	ui->subsriptionTableWidget->setHorizontalHeaderLabels(header);
	ui->subsriptionTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->subsriptionTableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->subsriptionTableWidget->verticalHeader()->setVisible(false);

	if(m_shvServerNode) {
		m_ServerProps = m_shvServerNode->serverProperties();
		QVariant v = m_ServerProps.value("subscriptions");
		if(v.isValid()) {
			m_subscriptionList = v.toList();
			for (int i = 0; i < m_subscriptionList.size(); i++) {
				QStringList subscription = m_subscriptionList.at(i).toStringList();

				shvInfo() << "LOAD" << subscription.at(0) << subscription.at(1);
				ui->subsriptionTableWidget->insertRow(i);
				ui->subsriptionTableWidget->setRowHeight(i, ui->subsriptionTableWidget->fontInfo().pointSize());
				ui->subsriptionTableWidget->setItem(i, 0, new QTableWidgetItem(subscription.at(0)));
				ui->subsriptionTableWidget->setItem(i, 1, new QTableWidgetItem(subscription.at(1)));
			}
		}
	}
	ui->subsriptionTableWidget->resizeColumnsToContents();

	connect(ui->addSubsriptionButton,	&QToolButton::clicked, this, &DlgSubscription::onSubscriptionAddButtonClicked);
	connect(ui->deleteSubscriptionButton, &QToolButton::clicked, this, &DlgSubscription::onSubscriptionDeleteButtonClicked);
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &DlgSubscription::onDialogOkButtonClicked);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, [this](void){ close(); });
}

DlgSubscription::~DlgSubscription()
{
	delete ui;
}

void DlgSubscription::onDialogOkButtonClicked()
{
	QSettings settings;
	settings.setValue(QStringLiteral("ui/dlgSubscription/geometry"), saveGeometry());
	m_shvServerNode->setServerSubscriptionProperties(m_ServerProps);
	m_srvTreeModel->saveSettings(settings);
	close();
}

void DlgSubscription::onSubscriptionDeleteButtonClicked()
{
	QModelIndexList selection = ui->subsriptionTableWidget->selectionModel()->selectedRows();
	if (selection.size() > 0){
		QModelIndex index = selection.at(0);
		ui->subsriptionTableWidget->removeRow(index.row());
		m_subscriptionList.removeAt(index.row());
	}
}

void DlgSubscription::onSubscriptionAddButtonClicked()
{
	m_shvServerNode->callCreateSubscription(ui->subscriptionPathLineEdit->text().toStdString(), ui->subscriptionMethodLineEdit->text().toStdString());
	ui->subsriptionTableWidget->insertRow(0);
	ui->subsriptionTableWidget->setRowHeight(0, ui->subsriptionTableWidget->fontInfo().pointSize());
	ui->subsriptionTableWidget->setItem(0, 0, new QTableWidgetItem(ui->subscriptionPathLineEdit->text()));
	ui->subsriptionTableWidget->setItem(0, 1, new QTableWidgetItem(ui->subscriptionMethodLineEdit->text()));

	QStringList subscription;
	subscription << ui->subscriptionPathLineEdit->text() << ui->subscriptionMethodLineEdit->text();

	m_ServerProps = m_shvServerNode->serverProperties();
	m_subscriptionList.insert(0, subscription);
	shvInfo() << m_subscriptionList.size();
	if (!m_ServerProps.contains("subscriptions")){
		m_ServerProps.insert("subscriptions", m_subscriptionList);
	} else {
		m_ServerProps["subscriptions"] = m_subscriptionList;
	}
	shvInfo() << "ADD" << subscription.at(0) << subscription.at(1);
	ui->subsriptionTableWidget->resizeColumnsToContents();
}
