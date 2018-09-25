#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "theapp.h"
#include "attributesmodel/attributesmodel.h"
#include "servertreemodel/servertreemodel.h"
#include "servertreemodel/shvbrokernodeitem.h"
#include "log/rpcnotificationsmodel.h"

//#include "dlgdumpnode.h"
#include "dlgserverproperties.h"
#include "dlgsubscriptionparameters.h"
#include "dlgsubscriptions.h"
#include "resultview.h"

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

namespace cp = shv::chainpack;

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	//setWindowTitle(tr("QFreeOpcUa Spy"));
	setWindowIcon(QIcon(":/shvspy/images/qfopcuaspy-256x256.png"));

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

	ui->tblAttributes->setModel(TheApp::instance()->attributesModel());
	ui->tblAttributes->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	ui->tblAttributes->verticalHeader()->setDefaultSectionSize(fontMetrics().height() * 1.3);
	ui->tblAttributes->setContextMenuPolicy(Qt::CustomContextMenu);


	connect(ui->tblAttributes, &QTableView::customContextMenuRequested, this, &MainWindow::attributesTableContexMenu);

	connect(ui->tblAttributes, &QTableView::activated, [](const QModelIndex &ix) {
		if(ix.column() == AttributesModel::ColBtRun)
			TheApp::instance()->attributesModel()->callMethod(ix.row());
	});
	connect(ui->tblAttributes, &QTableView::doubleClicked, [this](const QModelIndex &ix) {
		if (ix.column() == AttributesModel::ColResult) {
			displayResult(ix);
		}
	});


	ui->notificationsLogWidget->setLogTableModel(TheApp::instance()->rpcNotificationsModel());

	//ui->tblSubscriptions->setModel(TheApp::instance()->subscriptionsModel());
	QSettings settings;
	restoreGeometry(settings.value(QStringLiteral("ui/mainWindow/geometry")).toByteArray());
	restoreState(settings.value(QStringLiteral("ui/mainWindow/state")).toByteArray());
	TheApp::instance()->serverTreeModel()->loadSettings(settings);
}

MainWindow::~MainWindow()
{
	//TheApp::instance()->subscriptionsModel()->clear();
	//shvWarning() << __FUNCTION__;
	delete ui;
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
/*
void MainWindow::on_actSubscribeNodeValue_triggered()
{
	QModelIndex ix = ui->treeServers->currentIndex();
	ShvNodeItem *nd = dynamic_cast<ShvNodeItem*>(TheApp::instance()->serverTreeModel()->itemFromIndex(ix));
	ServerNode *snd = dynamic_cast<ServerNode*>(nd);
	if(nd && !snd) {
		qfopcua::SubscriptionParameters params;
		{
			DlgSubscriptionParameters dlg(this);
			dlg.setSubscriptionParameters(params);
			if(dlg.exec()) {
				params = dlg.subscriptionParameters();
			}
			else {
				return;
			}
		}
		snd = nd->serverNode();
		qfopcua::Subscription *subs = snd->clientConnection()->createSubscription(params);
		if(subs) {
			connect(subs, &qfopcua::Subscription::dataChanged, this, &MainWindow::onSubscribedDataChanged);
			qfopcua::Subscription::MonitoredItemId hnd = subs->subscribeDataChange(nd->nodeId());
			shvInfo() << "created subscription:" << subs << "subscribed data change handle:" << hnd << "node:" << nd->nodeId().toString();
			TheApp::instance()->subscriptionsModel()->addItem(subs, hnd);
		}
	}
}

void MainWindow::on_actDumpNode_triggered()
{
	QModelIndex ix = ui->treeServers->currentIndex();
	ShvNodeItem *nd = TheApp::instance()->serverTreeModel()->itemFromIndex(ix);
	ShvBrokerNodeItem *snd = qobject_cast<ShvBrokerNodeItem*>(nd);
	if(nd && !snd) {
		DlgDumpNode dlg(nd, this);
		dlg.generateText();
		dlg.exec();
	}
}
*/
void MainWindow::on_treeServers_customContextMenuRequested(const QPoint &pos)
{
	QModelIndex ix = ui->treeServers->indexAt(pos);
	ShvNodeItem *nd = TheApp::instance()->serverTreeModel()->itemFromIndex(ix);
	ShvBrokerNodeItem *snd = qobject_cast<ShvBrokerNodeItem*>(nd);
	QMenu m;
	QAction *a_reloadNode = new QAction(tr("Reload"), &m);
	QAction *a_subscribeNode = new QAction(tr("Subscribe"), &m);
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
	}
	if(!m.actions().isEmpty()) {
		QAction *a = m.exec(ui->treeServers->viewport()->mapToGlobal(pos));
		if(a) {
			if(a == a_reloadNode) {
				ShvNodeItem *nd = TheApp::instance()->serverTreeModel()->itemFromIndex(ui->treeServers->currentIndex());
				if(nd)
					nd->reload();
			}
			if(a == a_subscribeNode) {
				ShvNodeItem *nd = TheApp::instance()->serverTreeModel()->itemFromIndex(ui->treeServers->currentIndex());
				if(nd) {
					DlgSubscriptions dlg(this);
					QVariantMap props = nd->serverNode()->serverProperties();
					dlg.setSubscriptionsList(props.value(QStringLiteral("subscriptions")).toList());
					dlg.setShvPath(nd->shvPath());
					if (dlg.exec()){
						nd->serverNode()->setSubscriptionList(dlg.subscriptionsList());
					}
				}
			}
		}
	}
}

void MainWindow::openNode(const QModelIndex &ix)
{
	//shvInfo() << QF_FUNC_NAME;
	ShvNodeItem *nd = TheApp::instance()->serverTreeModel()->itemFromIndex(ix);
	ShvBrokerNodeItem *bnd = qobject_cast<ShvBrokerNodeItem*>(nd);
	if(bnd) {
		if(bnd->isOpen())
			bnd->close();
		else
			bnd->open();
	}
}

void MainWindow::displayResult(const QModelIndex &ix)
{
	//QApplication::setOverrideCursor(Qt::WaitCursor);
	QVariant v = ix.data(AttributesModel::RawResultRole);
	cp::RpcValue rv = qvariant_cast<cp::RpcValue>(v);

	std::string formatted;

	if(rv.isString()) {
		formatted = rv.toString();
	}
	else try {
		std::ostringstream pout;
		shv::chainpack::CponWriterOptions opts;
		opts.setIndent("\t");
		opts.setTranslateIds(true);
		{ shv::chainpack::CponWriter pwr(pout, opts); pwr.write(rv); }
		formatted = pout.str();
	}
	catch (std::exception &e) {
		formatted = e.what();
	}

	ResultView view(this);
	view.setText(QString::fromStdString(formatted));
	//QApplication::restoreOverrideCursor();
	view.exec();
}

void MainWindow::attributesTableContexMenu(const QPoint &point)
{
	QModelIndex index = ui->tblAttributes->indexAt(point);
	if (index.isValid() && index.column() == AttributesModel::ColResult) {
		QMenu menu(this);
		menu.addAction(tr("View result"));
		if (menu.exec(ui->tblAttributes->viewport()->mapToGlobal(point))) {
			displayResult(index);
		}
	}
}

void MainWindow::onShvTreeViewCurrentSelectionChanged(const QModelIndex &curr_ix, const QModelIndex &prev_ix)
{
	Q_UNUSED(prev_ix)
	ShvNodeItem *nd = TheApp::instance()->serverTreeModel()->itemFromIndex(curr_ix);
	{
		ShvBrokerNodeItem *bnd = qobject_cast<ShvBrokerNodeItem*>(nd);
		AttributesModel *m = TheApp::instance()->attributesModel();
		if(bnd) {
			// hide attributes for server nodes
			m->load(nullptr);
		}
		else {
			m->load(nd);
		}
	}
}
/*
void MainWindow::onSubscribedDataChanged(const qfopcua::DataValue &data_value, int att_id, const qfopcua::NodeId &node_id, qfopcua::Subscription::MonitoredItemId handle, qfopcua::Subscription::Id subscription_id)
{
	shvLogFuncFrame();
	qfDebug().noquote() << "DATA val:" << data_value.value() << data_value.toString() << "attr:" << qfopcua::AttributeId::toString(qfopcua::AttributeId::fromInt(att_id)) << "node:" << node_id.toString() << "handle:" << handle << "subscriber id:" << subscription_id;
}
*/
/*
void MainWindow::showOpcUaError(const QString &what)
{
	//qfWarning() << what;
	QMessageBox::critical(this, tr("Opc UA Error"), what);
}
*/


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
	TheApp::instance()->serverTreeModel()->saveSettings(settings);
	Super::closeEvent(ev);
}




