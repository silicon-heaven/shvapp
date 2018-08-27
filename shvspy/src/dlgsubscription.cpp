#include "dlgsubscription.h"
#include "ui_dlgsubscription.h"

#include <QSettings>
#include <QDebug>
#include <QCompleter>
#include <QStringListModel>

namespace cp = shv::chainpack;

DlgSubscription::DlgSubscription(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DlgSubscription)
{
	ui->setupUi(this);
}

DlgSubscription::DlgSubscription(QWidget *parent, ServerTreeModel *model, ServerTreeView *view) :
	QDialog(parent),
	ui(new Ui::DlgSubscription){
	ui->setupUi(this);
	m_view = view;
	m_model = model;

	m_nd = model->itemFromIndex(view->currentIndex());
	m_snd = m_nd->serverNode();
	ui->addSubscriptionLine->setText(m_nd->shvPath().data());
	m_view->expand(m_model->indexFromItem(m_nd));

	ui->tableWidget->setRowCount(0);
	ui->tableWidget->setColumnCount(2);
	QStringList header;
	header << "Path" << "Method" << "";
	ui->tableWidget->setHorizontalHeaderLabels(header);
	ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
	ui->tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
	ui->tableWidget->verticalHeader()->setVisible(false);

	QSettings settings(m_nd->serverNode()->nodeId().data());
	int size = settings.beginReadArray("Subscriptions");
	for (int i = 0; i < size; i++) {
		settings.setArrayIndex(i);
		Subscription sub;
		sub.path = settings.value("path").toString();
		sub.method = settings.value("method").toString();
		qDebug() << "LOAD" << sub.path << sub.method;
		subscriptions.insert(0, sub);
		ui->tableWidget->insertRow(i);
		ui->tableWidget->setRowHeight(i, ui->tableWidget->fontInfo().pointSize());
		ui->tableWidget->setItem(i, 0, new QTableWidgetItem(sub.path));
		ui->tableWidget->setItem(i, 1, new QTableWidgetItem(sub.method));
	}
	settings.endArray();

	connect(ui->addSubscriptionLine, &QLineEdit::cursorPositionChanged, this, &DlgSubscription::lineChanged);

	connect(ui->addSubscription, &QToolButton::clicked, this, &DlgSubscription::addSubscription);
	connect(ui->delSubscription, &QToolButton::clicked, this, &DlgSubscription::delSubscription);
	connect(ui->doneSubscription, &QToolButton::clicked, this, &DlgSubscription::doneSubscription);
}

DlgSubscription::~DlgSubscription()
{
	delete ui;
}

void DlgSubscription::doneSubscription(){
	QSettings settings(m_nd->serverNode()->nodeId().data());
	settings.beginWriteArray("Subscriptions");
	for (int i = 0; i < subscriptions.size(); ++i) {
		settings.setArrayIndex(i);
		settings.setValue("path", subscriptions.at(i).path);
		settings.setValue("method", subscriptions.at(i).method);
	}
	settings.endArray();
	close();
}

void DlgSubscription::delSubscription(){
	ui->returnText->setText("");
	QModelIndexList selection = ui->tableWidget->selectionModel()->selectedRows();
	if (selection.size() > 0){
		QModelIndex index = selection.at(0);
		ui->tableWidget->removeRow(index.row());
		subscriptions.removeAt(index.row());
		ui->returnText->setText("Subscription was removed.");
	}
}

void DlgSubscription::addSubscription(){
	m_rqid = m_snd->callCreateSubscription(ui->addSubscriptionLine->text().toStdString(), ui->lineMethod->text().toStdString());
	connect(m_snd, &ShvBrokerNodeItem::receiveRpcResponse, [this](const unsigned &id, const unsigned &rv){
		if (m_rqid != id) {
			return;
		}
		if (rv == 0){
			ui->returnText->setText("Subscription was added.");
			ui->tableWidget->insertRow(0);
			ui->tableWidget->setRowHeight(0, ui->tableWidget->fontInfo().pointSize());
			ui->tableWidget->setItem(0, 0, new QTableWidgetItem(ui->addSubscriptionLine->text()));
			ui->tableWidget->setItem(0, 1, new QTableWidgetItem(ui->lineMethod->text()));
			Subscription sub;
			sub.path = ui->addSubscriptionLine->text();
			sub.method = ui->lineMethod->text();
			subscriptions.insert(0, sub);
			qDebug() << "ADD" << sub.path << sub.method;
		} else {
			ui->returnText->setText("Cannot find path.");
		}
	});
}

void DlgSubscription::lineChanged(unsigned int oldPos, unsigned int newPos){
	ui->returnText->setText("");
	m_view->expand(m_model->indexFromItem(m_nd));
	if (m_nd->serverNode() == m_nd){
		showChildrenPopup();
		return;
	}
	if (m_nd->parentNode()->shvPath().length() == newPos) return;
	if (m_nd->shvPath().length() > newPos){
		ui->addSubscriptionLine->setText(m_nd->parentNode()->shvPath().data());
		m_view->collapse(m_model->indexFromItem(m_nd));
		m_nd = m_nd->parentNode();
		if (m_nd->nodeId() != m_nd->serverNode()->nodeId())
			m_view->collapse(m_model->indexFromItem(m_nd));
	}
	if ((oldPos+1) == newPos){
		if (m_nd->shvPath().length() != newPos)
			showChildrenPopup();
	}
}

void DlgSubscription::showChildrenPopup(){
	QComboBox * combo = new QComboBox(ui->addSubscriptionLine);

	for (int i = 0; i < m_nd->childCount(); i++)
		combo->addItem(m_nd->childAt(i)->nodeId().data(), i);
	combo->showPopup();
	connect(combo, QOverload<const QString &>::of(&QComboBox::activated),
			[=](const QString &text){
		int i = combo->currentIndex();
		m_nd = m_nd->childAt(i);
		ui->addSubscriptionLine->setText(m_nd->shvPath().data());
		m_view->expand(m_model->indexFromItem(m_nd));
	});
}
