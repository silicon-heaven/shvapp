#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "theapp.h"
#include "attributesmodel/attributesmodel.h"
#include "servertreemodel/servertreemodel.h"
#include "servertreemodel/shvbrokernodeitem.h"
#include "log/rpcnotificationsmodel.h"
#include "log/errorlogmodel.h"
#include "dlgserverproperties.h"
#include "dlgsubscriptionparameters.h"
#include "dlgcallshvmethod.h"
#include "methodparametersdialog.h"
#include "texteditdialog.h"

#include <shv/chainpack/chainpackreader.h>
#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/cponwriter.h>

//#include <qfopcua/client.h>

#include <shv/coreqt/log.h>

#include <shv/iotqt/rpc/rpc.h>

#include <QSettings>
#include <QMessageBox>
#include <QItemSelectionModel>
#include <QInputDialog>
#include <QScrollBar>

#include <fstream>

namespace cp = shv::chainpack;

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	addAction(ui->actionQuit);
	connect(ui->actionQuit, &QAction::triggered, TheApp::instance(), &TheApp::quit);
	//setWindowTitle(tr("QFreeOpcUa Spy"));
	setWindowIcon(QIcon(":/shvspy/images/shvspy"));

	ui->menu_View->addAction(ui->dockServers->toggleViewAction());
	ui->menu_View->addAction(ui->dockAttributes->toggleViewAction());
	ui->menu_View->addAction(ui->dockNotifications->toggleViewAction());
	ui->menu_View->addAction(ui->dockErrors->toggleViewAction());
	ui->menu_View->addAction(ui->dockSubscriptions->toggleViewAction());

	ServerTreeModel *tree_model = TheApp::instance()->serverTreeModel();
	ui->treeServers->setModel(tree_model);
	connect(tree_model, &ServerTreeModel::dataChanged, ui->treeServers,[this](const QModelIndex &tl, const QModelIndex &br, const QVector<int> &roles) {
		/// expand broker node when children loaded
		Q_UNUSED(roles)
		if(tl == br) {
			ServerTreeModel *tree_model = TheApp::instance()->serverTreeModel();
			ShvBrokerNodeItem *brit = qobject_cast<ShvBrokerNodeItem*>(tree_model->itemFromIndex(tl));
			if(brit) {
				if(tree_model->hasChildren(tl)) {
					ui->treeServers->expand(tl);
				}
			}
		}
	});
	connect(ui->treeServers->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::onShvTreeViewCurrentSelectionChanged);
	connect(tree_model, &ServerTreeModel::brokerConnectedChange, ui->subscriptionsWidget, &SubscriptionsWidget::onBrokerConnectedChanged);
	connect(tree_model, &ServerTreeModel::subscriptionAdded, ui->subscriptionsWidget, &SubscriptionsWidget::onSubscriptionAdded);

	ui->tblAttributes->setModel(TheApp::instance()->attributesModel());
	ui->tblAttributes->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	ui->tblAttributes->verticalHeader()->setDefaultSectionSize(fontMetrics().height() * 1.3);
	ui->tblAttributes->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(TheApp::instance()->attributesModel(), &AttributesModel::reloaded, this, &MainWindow::resizeAttributesViewSectionsToFit);
	connect(TheApp::instance()->attributesModel(), &AttributesModel::methodCallResultChanged, this, [this](int method_ix) {
		Q_UNUSED(method_ix)
		this->resizeAttributesViewSectionsToFit();
	});
	connect(ui->tblAttributes, &QTableView::customContextMenuRequested, this, &MainWindow::onAttributesTableContexMenu);

	connect(ui->tblAttributes, &QTableView::activated, [](const QModelIndex &ix) {
		if(ix.column() == AttributesModel::ColBtRun)
			TheApp::instance()->attributesModel()->callMethod(ix.row());
	});
	connect(ui->tblAttributes, &QTableView::doubleClicked, this, [this](const QModelIndex &ix) {
		if (ix.column() == AttributesModel::ColResult) {
			displayResult(ix);
		}
		else if (ix.column() == AttributesModel::ColParams) {
			editCponParameters(ix);
		}
	}, Qt::QueuedConnection);


	ui->notificationsLogWidget->setLogTableModel(TheApp::instance()->rpcNotificationsModel());
	ui->errorLogWidget->setLogTableModel(TheApp::instance()->errorLogModel());

	QSettings settings;
	restoreGeometry(settings.value(QStringLiteral("ui/mainWindow/geometry")).toByteArray());
	restoreState(settings.value(QStringLiteral("ui/mainWindow/state")).toByteArray());
	TheApp::instance()->loadSettings(settings);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::resizeAttributesViewSectionsToFit()
{
	QHeaderView *hh = ui->tblAttributes->horizontalHeader();
	hh->resizeSections(QHeaderView::ResizeToContents);
	int sum_section_w = 0;
	for (int i = 0; i < hh->count(); ++i)
		sum_section_w += hh->sectionSize(i);
	int widget_w = ui->tblAttributes->geometry().size().width();
	if(sum_section_w - widget_w > 0) {
		int w_params = hh->sectionSize(AttributesModel::ColParams);
		int w_result = hh->sectionSize(AttributesModel::ColResult);
		int w_section_rest = sum_section_w - w_params - w_result;
		int w_params2 = w_params * (widget_w - w_section_rest) / (w_params + w_result);
		int w_result2 = w_result * (widget_w - w_section_rest) / (w_params + w_result);
		//shvDebug() << "widget:" << widget_w << "com col w:" << sum_section_w << "params section size:" << w_params << "result section size:" << w_result;
		hh->resizeSection(AttributesModel::ColParams, w_params2);
		hh->resizeSection(AttributesModel::ColResult, w_result2);
	}
}

void MainWindow::on_actAddServer_triggered()
{
	editServer(nullptr, false);
}

void MainWindow::on_actEditServer_triggered()
{
	QModelIndex ix = ui->treeServers->currentIndex();
	ShvNodeItem *nd = TheApp::instance()->serverTreeModel()->itemFromIndex(ix);
	ShvBrokerNodeItem *brnd = qobject_cast<ShvBrokerNodeItem*>(nd);
	if(brnd) {
		editServer(brnd, false);
	}
}

void MainWindow::on_actCopyServer_triggered()
{
	QModelIndex ix = ui->treeServers->currentIndex();
	ShvNodeItem *nd = TheApp::instance()->serverTreeModel()->itemFromIndex(ix);
	ShvBrokerNodeItem *brnd = qobject_cast<ShvBrokerNodeItem*>(nd);
	if(brnd) {
		editServer(brnd, true);
	}
}

void MainWindow::on_actRemoveServer_triggered()
{
	QModelIndex ix = ui->treeServers->currentIndex();
	ShvNodeItem *nd = TheApp::instance()->serverTreeModel()->itemFromIndex(ix);
	ShvBrokerNodeItem *brnd = qobject_cast<ShvBrokerNodeItem*>(nd);
	if(brnd) {
		if(QMessageBox::question(this, tr("Question"), tr("Realy drop server definition for '%1'").arg(nd->objectName())) == QMessageBox::Yes) {
			TheApp::instance()->serverTreeModel()->invisibleRootItem()->deleteChild(ix.row());
		}
	}
}

void MainWindow::on_treeServers_customContextMenuRequested(const QPoint &pos)
{
	QModelIndex ix = ui->treeServers->indexAt(pos);
	ShvNodeItem *nd = TheApp::instance()->serverTreeModel()->itemFromIndex(ix);
	ShvBrokerNodeItem *snd = qobject_cast<ShvBrokerNodeItem*>(nd);
	QMenu m;
	QAction *a_reloadNode = new QAction(tr("Reload"), &m);
	QAction *a_subscribeNode = new QAction(tr("Subscribe"), &m);
	QAction *a_callShvMethod = new QAction(tr("Call shv method"), &m);
	//QAction *a_test = new QAction(tr("create test.txt"), &m);
	if(!nd) {
		m.addAction(ui->actAddServer);
	}
	else if(snd) {
		m.addAction(ui->actAddServer);
		m.addAction(ui->actEditServer);
		m.addAction(ui->actCopyServer);
		m.addAction(ui->actRemoveServer);
		if(snd->isOpen()) {
			m.addSeparator();
			m.addAction(a_reloadNode);
		}
	}
	else {
		m.addAction(a_reloadNode);
		m.addAction(a_subscribeNode);
		m.addAction(a_callShvMethod);
	}
	if(!m.actions().isEmpty()) {
		QAction *a = m.exec(ui->treeServers->viewport()->mapToGlobal(pos));
		if(a) {
			if(a == a_reloadNode) {
				ShvNodeItem *nd = TheApp::instance()->serverTreeModel()->itemFromIndex(ui->treeServers->currentIndex());
				if(nd)
					nd->reload();
			}
			else if(a == a_subscribeNode) {
				ShvNodeItem *nd = TheApp::instance()->serverTreeModel()->itemFromIndex(ui->treeServers->currentIndex());
				if(nd) {
					nd->serverNode()->addSubscription(nd->shvPath(), "change");
				}
			}
			else if(a == a_callShvMethod) {
				ShvNodeItem *nd = TheApp::instance()->serverTreeModel()->itemFromIndex(ui->treeServers->currentIndex());
				if(nd) {
					shv::iotqt::rpc::ClientConnection *cc = nd->serverNode()->clientConnection();

					DlgCallShvMethod dlg(cc, this);
					dlg.setShvPath(nd->shvPath());
					dlg.exec();
				}
			}
		}
	}
}

void MainWindow::openNode(const QModelIndex &ix)
{
	ShvNodeItem *nd = TheApp::instance()->serverTreeModel()->itemFromIndex(ix);
	ShvBrokerNodeItem *bnd = qobject_cast<ShvBrokerNodeItem*>(nd);
	if(bnd) {
		AttributesModel *m = TheApp::instance()->attributesModel();
		if(bnd->openStatus() == ShvBrokerNodeItem::OpenStatus::Disconnected) {
			bnd->open();
		}
		else {
			bnd->close();
			m->load(nullptr);
		}
	}
}

void MainWindow::displayResult(const QModelIndex &ix)
{
	//QApplication::setOverrideCursor(Qt::WaitCursor);
	QVariant v = ix.data(AttributesModel::RpcValueRole);
	cp::RpcValue rv = qvariant_cast<cp::RpcValue>(v);
	if(rv.isString()) {
		TextEditDialog *view = new TextEditDialog(this);
		view->setModal(false);
		view->setAttribute(Qt::WA_DeleteOnClose);
		view->setWindowIconText(tr("Result"));
		view->setReadOnly(true);
		view->setText(QString::fromStdString(rv.toString()));
		view->show();
	}
	else {
		CponEditDialog *view = new CponEditDialog(this);
		view->setModal(false);
		view->setAttribute(Qt::WA_DeleteOnClose);
		view->setWindowIconText(tr("Result"));
		view->setReadOnly(true);
		view->setValidateContent(false);
		QString cpon = QString::fromStdString(rv.toCpon("  "));
		view->setText(cpon);
		view->show();
	}
}

void MainWindow::editMethodParameters(const QModelIndex &ix)
{
	QVariant v = ix.data(AttributesModel::RpcValueRole);
	cp::RpcValue rv = qvariant_cast<cp::RpcValue>(v);

	QString path = TheApp::instance()->attributesModel()->path();
	QString method = TheApp::instance()->attributesModel()->method(ix.row());
	MethodParametersDialog dlg(path, method, rv, this);
	dlg.setWindowTitle(tr("Parameters"));
	if (dlg.exec() == QDialog::Accepted) {
		cp::RpcValue val = dlg.value();
		if (val.isValid()) {
			std::string cpon = dlg.value().toCpon();
			ui->tblAttributes->model()->setData(ix, QString::fromStdString(cpon), Qt::EditRole);
		}
		else {
			ui->tblAttributes->model()->setData(ix, QString(), Qt::EditRole);
		}
	}
}

void MainWindow::editStringParameter(const QModelIndex &ix)
{
	QVariant v = ix.data(AttributesModel::RpcValueRole);
	cp::RpcValue rv = qvariant_cast<cp::RpcValue>(v);
	QString cpon = QString::fromStdString(rv.toString());
	TextEditDialog dlg(this);
	dlg.setWindowTitle(tr("Parameters"));
	dlg.setReadOnly(false);
	dlg.setText(cpon);
	if(dlg.exec()) {
		rv = cp::RpcValue(dlg.text().toStdString());
		cpon =  QString::fromStdString(rv.toCpon());
		ui->tblAttributes->model()->setData(ix, cpon, Qt::EditRole);
	}
}

void MainWindow::editCponParameters(const QModelIndex &ix)
{
	QVariant v = ix.data(AttributesModel::RpcValueRole);
	cp::RpcValue rv = qvariant_cast<cp::RpcValue>(v);
	QString cpon = rv.isValid()? QString::fromStdString(rv.toCpon("  ")): QString();
	CponEditDialog dlg(this);
	dlg.setWindowTitle(tr("Parameters"));
	dlg.setReadOnly(false);
	dlg.setValidateContent(true);
	dlg.setText(cpon);
	if(dlg.exec()) {
		cpon = dlg.text();
		ui->tblAttributes->model()->setData(ix, cpon, Qt::EditRole);
	}
}

void MainWindow::onAttributesTableContexMenu(const QPoint &point)
{
	QModelIndex index = ui->tblAttributes->indexAt(point);
	if (index.isValid() && index.column() == AttributesModel::ColResult) {
		QMenu menu(this);
		menu.addAction(tr("View result"));
		if (menu.exec(ui->tblAttributes->viewport()->mapToGlobal(point))) {
			displayResult(index);
		}
	}
	else if (index.isValid() && index.column() == AttributesModel::ColParams) {
		QMenu menu(this);
		QAction *a_par_ed = menu.addAction(tr("Parameters editor"));
		QAction *a_str_ed = menu.addAction(tr("String parameter editor"));
		QAction *a_cpon_ed = menu.addAction(tr("Cpon parameters editor"));
		QAction *a = menu.exec(ui->tblAttributes->viewport()->mapToGlobal(point));
		if (a == a_par_ed) {
			editMethodParameters(index);
		}
		else if (a == a_str_ed) {
			editStringParameter(index);
		}
		else if (a == a_cpon_ed) {
			editCponParameters(index);
		}
	}
}

void MainWindow::onShvTreeViewCurrentSelectionChanged(const QModelIndex &curr_ix, const QModelIndex &prev_ix)
{
	Q_UNUSED(prev_ix)
	ShvNodeItem *nd = TheApp::instance()->serverTreeModel()->itemFromIndex(curr_ix);
	{
		AttributesModel *m = TheApp::instance()->attributesModel();
		ui->edAttributesShvPath->setText(QString::fromStdString(nd->shvPath()));
		ShvBrokerNodeItem *bnd = qobject_cast<ShvBrokerNodeItem*>(nd);
		if(bnd) {
			// hide attributes for server nodes
			//ui->edAttributesShvPath->setText(QString());
			m->load(bnd->isOpen()? bnd: nullptr);
		}
		else {
			//ui->edAttributesShvPath->setText(QString::fromStdString(nd->shvPath()));
			m->load(nd);
		}
	}
}

void MainWindow::editServer(ShvBrokerNodeItem *srv, bool copy_server)
{
	shvLogFuncFrame() << srv;
	QVariantMap server_props;
	if(srv) {
		server_props = srv->serverProperties();
	}
	DlgServerProperties dlg(this);
	dlg.setServerProperties(server_props);
	if(dlg.exec()) {
		server_props = dlg.serverProperties();
		if(!srv || copy_server)
			TheApp::instance()->serverTreeModel()->createConnection(server_props);
		else
			srv->setServerProperties(server_props);
	}
}

void MainWindow::closeEvent(QCloseEvent *ev)
{
	QSettings settings;
	settings.setValue(QStringLiteral("ui/mainWindow/state"), saveState());
	settings.setValue(QStringLiteral("ui/mainWindow/geometry"), saveGeometry());
	TheApp::instance()->saveSettings(settings);
	Super::closeEvent(ev);
}
