#include "dlglogview.h"
#include "ui_dlglogview.h"

#include "logmodel.h"

#include "../jn50viewapp.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/core/utils/shvjournalgetlogparams.h>
#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/utils.h>
#include <shv/visu/timeline/graphmodel.h>
#include <shv/visu/timeline/graphwidget.h>
#include <shv/visu/timeline/graph.h>
#include <shv/coreqt/log.h>

#include <QSettings>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>

namespace cp = shv::chainpack;
namespace tl = shv::visu::timeline;

DlgLogView::DlgLogView(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DlgLogView)
{
	ui->setupUi(this);
	ui->lblInfo->hide();
	ui->btMoreOptions->setChecked(false);
	QDateTime dt2 = QDateTime::currentDateTime();
	QDateTime dt1 = dt2.addDays(-1);
	dt2 = dt2.addSecs(60 * 60);
	ui->edSince->setDateTime(dt1);
	ui->edUntil->setDateTime(dt2);

	m_logModel = new LogModel(this);
	m_sortFilterProxy = new QSortFilterProxyModel(this);
	m_sortFilterProxy->setFilterKeyColumn(-1);
	m_sortFilterProxy->setSourceModel(m_logModel);
	ui->tblData->setModel(m_sortFilterProxy);
	ui->tblData->setSortingEnabled(true);
	connect(ui->edFilter, &QLineEdit::textChanged, [this]() {
		QString str = ui->edFilter->text().trimmed();
		m_sortFilterProxy->setFilterFixedString(str);
		//if(str.isEmpty()) {
		//}
		//m_sortFilterProxy->setFilterWildcard("*" + str + "*");
	});

	m_graphModel = new tl::GraphModel(this);
	//connect(m_dataModel, &tl::GraphModel::xRangeChanged, this, &MainWindow::onGraphXRangeChanged);
	//ui->graphView->viewport()->show();
	m_graphWidget = new tl::GraphWidget();

	ui->graphView->setBackgroundRole(QPalette::Dark);
	ui->graphView->setWidget(m_graphWidget);
	ui->graphView->widget()->setBackgroundRole(QPalette::ToolTipBase);
	ui->graphView->widget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

	//shvInfo() << qobject_cast<QWidget*>(ui->graphView->widget());
	m_graph = new tl::Graph(this);
	m_graph->setModel(m_graphModel);
	m_graphWidget->setGraph(m_graph);

	connect(ui->btLoad, &QPushButton::clicked, this, &DlgLogView::downloadLog);

	loadSettings();
}

DlgLogView::~DlgLogView()
{
	saveSettings();
	delete ui;
}

void DlgLogView::loadSettings()
{
	QSettings settings;
	QByteArray ba = settings.value("ui/DlgLogView/geometry").toByteArray();
	restoreGeometry(ba);
}

void DlgLogView::saveSettings()
{
	QSettings settings;
	QByteArray ba = saveGeometry();
	settings.setValue("ui/DlgLogView/geometry", ba);
}

shv::iotqt::rpc::ClientConnection *DlgLogView::rpcConnection()
{
	Jn50ViewApp *app = Jn50ViewApp::instance();
	return app->rpcConnection();
}

shv::chainpack::RpcValue DlgLogView::getLogParams()
{
	shv::core::utils::ShvJournalGetLogParams params;
	auto get_dt = [](QDateTimeEdit *ed) {
		QDateTime dt = ed->dateTime();
		return dt == ed->minimumDateTime()?  cp::RpcValue():  cp::RpcValue::DateTime::fromMSecsSinceEpoch(ed->dateTime().toMSecsSinceEpoch());
	};
	params.since = get_dt(ui->edSince);
	params.until = get_dt(ui->edUntil);
	if(ui->edMaxRecordCount->value() > ui->edMaxRecordCount->minimum())
		params.maxRecordCount = ui->edMaxRecordCount->value();
	params.withUptime = ui->chkWithUptime->isChecked();
	params.withSnapshot = ui->chkWithSnapshot->isChecked();
	unsigned header_opts = 0;
	if(ui->chkBasicInfo->isChecked())
		header_opts |= static_cast<unsigned>(shv::core::utils::ShvJournalGetLogParams::HeaderOptions::BasicInfo);
	if(ui->chkFieldInfo->isChecked())
		header_opts |= static_cast<unsigned>(shv::core::utils::ShvJournalGetLogParams::HeaderOptions::FieldInfo);
	if(ui->chkTypeInfo->isChecked())
		header_opts |= static_cast<unsigned>(shv::core::utils::ShvJournalGetLogParams::HeaderOptions::TypeInfo);
	if(ui->chkPathsDict->isChecked())
		header_opts |= static_cast<unsigned>(shv::core::utils::ShvJournalGetLogParams::HeaderOptions::PathsDict);
	params.headerOptions = header_opts;
	return params.toRpcValue();
}

void DlgLogView::downloadLog()
{
	/*
	QProgressDialog *progressdlg = new QProgressDialog("Downloading log...", "Abort", 0, 0, this);
	progressdlg->setWindowModality(Qt::WindowModal);
	progressdlg->setMinimumDuration(0);
	progressdlg->setAutoClose(false);
	//progressdlg->open();
	*/

	std::string shv_path = shvPath().toStdString();
	if(ui->chkUseHistoryProvider->isChecked())
		shv_path = "history" + shv_path.substr(3);
	showInfo(QString::fromStdString("Downloading data from " + shv_path));
	cp::RpcValue params = getLogParams();
	shv::iotqt::rpc::ClientConnection *conn = rpcConnection();
	int rq_id = conn->nextRequestId();
	shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rq_id, this);
	cb->setTimeout(ui->edTimeout->value() * 1000);
	//connect(progressdlg, &QProgressDialog::canceled, [cb]() {
	//	cb->abort();
	//});
	cb->start(this, [this, shv_path](const cp::RpcResponse &resp) {
		if(resp.isValid()) {
			if(resp.isError()) {
				showInfo(QString::fromStdString("GET " + shv_path + " RPC request error: " + resp.error().toString()), true);
			}
			else {
				showInfo();
				this->parseLog(resp.result());
			}
		}
		else {
			showInfo(QString::fromStdString("GET " + shv_path + " RPC request timeout"), true);
		}
		//progressdlg->deleteLater();
	});
	conn->callShvMethod(rq_id, shv_path, cp::Rpc::METH_GET_LOG, params);
}

void DlgLogView::parseLog(shv::chainpack::RpcValue log)
{
	{
		std::string str = log.metaData().toString();
		ui->edInfo->setPlainText(QString::fromStdString(str));
	}
	{
		m_logModel->setLog(log);
		ui->tblData->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
	}
	{
		const shv::chainpack::RpcValue::IMap &dict = log.metaValue("pathsDict").toIMap();
		const shv::chainpack::RpcValue::List lst = log.toList();
		m_graphModel->clear();
		m_graphModel->beginAppendValues();
		for(const cp::RpcValue &rec : lst) {
			const cp::RpcValue::List &row = rec.toList();

			cp::RpcValue::DateTime dt = row.value(0).toDateTime();
			cp::RpcValue rv_path = row.value(1);
			if(rv_path.isUInt() || rv_path.isInt())
				rv_path = dict.value(rv_path.toInt());
			const shv::chainpack::RpcValue::String &path = rv_path.toString();
			if(path.empty()) {
				shvError() << "invalid entry path:" << rec.toCpon();
				continue;
			}
			cp::RpcValue rv = row.value(2);
			//cp::RpcValue short_time = row.value(3);
			int64_t msec = dt.msecsSinceEpoch();
			QVariant v = shv::iotqt::Utils::rpcValueToQVariant(rv);
			if(v.isValid())
				m_graphModel->appendValueShvPath(path, tl::Sample{msec, v});
		}
		m_graphModel->endAppendValues();
	}
	m_graph->createChannelsFromModel();
	ui->graphView->makeLayout();
}

void DlgLogView::showInfo(const QString &msg, bool is_error)
{
	if(msg.isEmpty()) {
		ui->lblInfo->hide();
	}
	else {
		QString ss = is_error? QString("background: salmon"): QString("background: lime");
		ui->lblInfo->setStyleSheet(ss);
		ui->lblInfo->setText(msg);
		ui->lblInfo->show();
	}
}

