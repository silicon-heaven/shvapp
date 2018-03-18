#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "theapp.h"
#include "attributesmodel/attributesmodel.h"
#include "servertreemodel/servertreemodel.h"
#include "servertreemodel/shvbrokernodeitem.h"
#include "log/rpcnotificationsmodel.h"

#include "dlgdumpnode.h"
#include "dlgserverproperties.h"
#include "dlgsubscriptionparameters.h"

//#include <qfopcua/client.h>

#include <shv/coreqt/log.h>

#include <QSettings>
#include <QMessageBox>
#include <QItemSelectionModel>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	//setWindowTitle(tr("QFreeOpcUa Spy"));
	setWindowIcon(QIcon(":/shvspy/images/qfopcuaspy-256x256.png"));

	ui->treeServers->setModel(TheApp::instance()->serverTreeModel());
	connect(ui->treeServers->selectionModel(), &QItemSelectionModel::currentChanged, this, &MainWindow::onShvTreeViewCurrentSelectionChanged);

	ui->tblAttributes->setModel(TheApp::instance()->attributesModel());
	ui->tblAttributes->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	ui->tblAttributes->verticalHeader()->setDefaultSectionSize(fontMetrics().height() * 1.3);

	connect(ui->tblAttributes, &QTableView::activated, [this](const QModelIndex &ix) {
		if(ix.column() == AttributesModel::ColBtRun)
			TheApp::instance()->attributesModel()->callMethod(ix.row());
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
		if(QMessageBox::question(this, tr("Question"), tr("Realy drop server deinition for '%1'").arg(nd->objectName())) == QMessageBox::Yes) {
			nd = TheApp::instance()->serverTreeModel()->invisibleRootItem()->takeChild(ix.row());
			delete nd;
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
*/
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

void MainWindow::on_treeServers_customContextMenuRequested(const QPoint &pos)
{
	QModelIndex ix = ui->treeServers->indexAt(pos);
	ShvNodeItem *nd = TheApp::instance()->serverTreeModel()->itemFromIndex(ix);
	ShvBrokerNodeItem *snd = qobject_cast<ShvBrokerNodeItem*>(nd);
	QMenu m;
	QAction *a_dumpEyasIds = nullptr;
	if(!nd) {
		m.addAction(ui->actAddServer);
	}
	else if(snd) {
		m.addAction(ui->actAddServer);
		m.addAction(ui->actEditServer);
		m.addAction(ui->actCopyServer);
		m.addAction(ui->actRemoveServer);
	}
	else {
		m.addAction(ui->actSubscribeNodeValue);
		m.addAction(ui->actDumpNode);
		a_dumpEyasIds = new QAction(tr("Dump EYAS IDs"), &m);
		m.addAction(a_dumpEyasIds);
	}
	if(!m.actions().isEmpty()) {
		QAction *a = m.exec(ui->treeServers->viewport()->mapToGlobal(pos));
		if(a) {
#if 0
			if(a == a_dumpEyasIds) {
				QStandardItem *it = TheApp::instance()->serverTreeModel()->itemFromIndex(ui->treeServers->currentIndex());
				ShvNodeItem *current_ua_nd = dynamic_cast<ShvNodeItem*>(it);
				if(current_ua_nd) {
					bool ok;
					QString browse_name = QInputDialog::getText(this, tr("Get text"),
														   tr("property to enumerate:"), QLineEdit::Normal,
														   "tc_dev.name", &ok);
					if(ok) {
						QStringList browse_name_path = browse_name.split('.');
						qfopcua::Client *ua_client = current_ua_nd->serverNode()->clientConnection();
						const qfopcua::NodeIdList lst = ua_client->getChildren(current_ua_nd->nodeId());
						for(const qfopcua::NodeId &parent_ndid : lst) {
							//() << "parent" << parent_ndid.toString() << "browser name:" << browse_name;
							qfopcua::NodeId child_ndid = ua_client->getChild(parent_ndid, browse_name_path);
							if(child_ndid.isNull())
								break;
							qfopcua::DataValue dv = ua_client->getAttribute(child_ndid, qfopcua::AttributeId::Value);
							shvInfo() << child_ndid.toString() << dv.value().toString();
						}
					}
				}
			}
#endif
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



