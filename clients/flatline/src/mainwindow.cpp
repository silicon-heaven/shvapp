#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "flatlineapp.h"
#include "appclioptions.h"
#include "timeline/graphmodel.h"
#include "timeline/graphwidget.h"

#include <shv/chainpack/chainpackreader.h>
#include <shv/chainpack/cponreader.h>
#include <shv/coreqt/log.h>
#include <shv/iotqt/utils.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/core/string.h>

#include <QFileDialog>

#include <QRandomGenerator>
#include <QTimer>
#include <fstream>
//#include <iostream>

namespace cp = shv::chainpack;

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	m_dataModel = new timeline::GraphModel(this);

	ui->setupUi(this);
	//ui->graphView->viewport()->show();
	m_graphWidget = new timeline::GraphWidget();

	ui->graphView->setBackgroundRole(QPalette::Dark);
	ui->graphView->setWidget(m_graphWidget);
	ui->graphView->widget()->setBackgroundRole(QPalette::ToolTipBase);
	ui->graphView->widget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

	//shvInfo() << qobject_cast<QWidget*>(ui->graphView->widget());
	m_graphWidget->setModel(m_dataModel);

	//timeline::Graph::GraphStyle style;
	//style.setColorBackground(QColor(Qt::darkGray).darker(400));
	//m_graphWidget->setStyle(style);

	FlatLineApp *app = FlatLineApp::instance();
	connect(app->rpcConnection(), &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &MainWindow::onRpcMessageReceived);
	//connect(this, &MainWindow::sendRpcMessage, app->rpcConnection(), &shv::iotqt::rpc::ClientConnection::sendMessage);
	if(!app->cliOptions()->logFile().empty()) {
		openLogFile(app->cliOptions()->logFile());
	}

	connect(ui->btGenerateSamples, &QPushButton::clicked, this, &MainWindow::generateRandomSamples);
	QTimer::singleShot(0, this, &MainWindow::generateRandomSamples);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::generateRandomSamples()
{
	timeline::GraphModel *model = m_dataModel;

	model->clear();
	model->appendChannel();
	model->setChannelData(0, "TC", timeline::GraphModel::ChannelDataRole::Name);
	model->appendChannel();
	model->setChannelData(1, "ADC", timeline::GraphModel::ChannelDataRole::Name);
	model->appendChannel();
	model->setChannelData(2, "Temp", timeline::GraphModel::ChannelDataRole::Name);
	model->beginAppendValues();
	int cnt = ui->edSamplesCount->value();
	int interval_msec = ui->edTimeInterval->value();
	QRandomGenerator *rnd = QRandomGenerator::global();
	int ch_ix = 0;
	{
		int64_t time = 0;
		for (int i = 0; i < cnt; ++i) {
			int t = rnd->bounded(interval_msec);
			int v = rnd->bounded(-2048, 2048);
			model->appendValue(ch_ix, timeline::Sample{time + t, v});
			time += t;
		}
		ch_ix++;
	}
	{
		int64_t time = 0;
		for (int i = 0; i < cnt; ++i) {
			int t = rnd->bounded(interval_msec);
			int v = rnd->bounded(2);
			model->appendValue(ch_ix, timeline::Sample{time + t, v});
			time += t;
		}
		ch_ix++;
	}
	{
		int64_t time = 0;
		for (int i = 0; i < cnt; ++i) {
			int t = rnd->bounded(interval_msec);
			double v = rnd->generateDouble() * 60 - 30;
			model->appendValue(ch_ix, timeline::Sample{time + t, v});
			time += t;
		}
		ch_ix++;
	}
	model->endAppendValues();

	timeline::Graph *gr = m_graphWidget->graph();
	gr->clearChannels();
	gr->setXRange(model->xRange());
	int ix = 0;
	{
		gr->appendChannel();
		timeline::Graph::Channel &ch = gr->channelAt(ix);
		timeline::Graph::ChannelStyle ch_style = ch.style();
		ch_style.setInterpolation(timeline::Graph::ChannelStyle::Interpolation::Stepped);
		ch_style.setColor(Qt::cyan);
		ch_style.setHeightMax(6);
		ch.setYRange(model->yRange(ix));
		ch.setStyle(ch_style);
		ix++;
	}
	{
		gr->appendChannel();
		timeline::Graph::Channel &ch = gr->channelAt(ix);
		timeline::Graph::ChannelStyle ch_style = ch.style();
		ch_style.setLineWidth(0.2);
		ch_style.setInterpolation(timeline::Graph::ChannelStyle::Interpolation::Stepped);
		ch_style.setColor(Qt::magenta);
		ch_style.setHeightMin(2);
		ch_style.setHeightMax(2);
		ch.setYRange(model->yRange(ix));
		ch.enlargeYRange(ch_style.lineWidth() / 2);
		ch.setStyle(ch_style);
		ix++;
	}
	{
		gr->appendChannel();
		timeline::Graph::Channel &ch = gr->channelAt(ix);
		timeline::Graph::ChannelStyle ch_style = ch.style();
		ch_style.setColor("orange");
		ch_style.setColorLineArea("orange");
		//ch_style.setHeightMax(6);
		ch.setYRange(model->yRange(ix));
		ch_style.setInterpolation(timeline::Graph::ChannelStyle::Interpolation::None);
		ch_style.setLineAreaStyle(timeline::Graph::ChannelStyle::LineAreaStyle::Filled);
		ch.setStyle(ch_style);
		ix++;
	}

	ui->graphView->makeLayout();
}

void MainWindow::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvDebug() << msg.toCpon();
	if(msg.isSignal()) {
		cp::RpcSignal ntf(msg);
		shvDebug() << "SIG:" << ntf.toCpon();
		if(shv::core::String::endsWith(ntf.shvPath().toString(), "/data") && ntf.method() == cp::Rpc::SIG_VAL_CHANGED) {
			if(!m_paused) {
				const cp::RpcValue::List& data = ntf.params().toList();
/*
				DataSample sample;
				sample.time = data.value(0).toUInt();
				sample.values[0] = data.value(1).toInt();
				sample.values[1] = data.value(2).toInt();
				sample.values[2] = data.value(3).toInt();

				addScanSample(data);

				ui->frmGraph->addSample(std::move(sample));
				*/
			}
		}
	}
}

void MainWindow::openLogFile(const std::string &fn)
{
	try {
		std::ifstream file_is;
		file_is.open(fn, std::ios::binary);
		shv::chainpack::AbstractStreamReader *rd;
		if(shv::core::String::endsWith(fn, ".chainpack"))
			rd = new shv::chainpack::ChainPackReader(file_is);
		else if(shv::core::String::endsWith(fn, ".cpon"))
			rd = new shv::chainpack::CponReader(file_is);
		else
			SHV_EXCEPTION("Unknown extension");
		shv::chainpack::RpcValue rv = rd->read();
		file_is.close();
		delete rd;
		setLogData(rv);
	}
	catch (shv::core::Exception &e) {
		shvError() << "Error reading file:" << fn << e.message();
	}
}

void MainWindow::setLogData(const shv::chainpack::RpcValue &data)
{
	if(m_dataModel)
		delete m_dataModel;
	m_dataModel = new timeline::GraphModel(this);
	m_graphWidget->setModel(m_dataModel);
	if(data.isList()) {
		addLogEntries(data.toList());
	}
}

void MainWindow::addLogEntries(const shv::chainpack::RpcValue::List &data)
{
	m_dataModel->beginAppendValues();
	QMap<std::string, int> path_cache;
	int max_path_id = -1;
	auto channel_index = [&path_cache, &max_path_id](const std::string &path) -> int {
		int ix = path_cache.value(path, -1);
		if(ix >= 0)
			return ix;
		ix = ++max_path_id;
		path_cache[path] = ix;
		return ix;
	};
	for(const cp::RpcValue &entry : data) {
		const cp::RpcValue::List &row = entry.toList();
		const cp::RpcValue::DateTime &dt = row.value(0).toDateTime();
		const cp::RpcValue::String &path = row.value(1).toString();
		const cp::RpcValue val = row.value(2);
		int chix = channel_index(path);
		while (m_dataModel->channelCount() <= chix) {
			m_dataModel->appendChannel();
		}
		QVariant v = shv::iotqt::Utils::rpcValueToQVariant(val);
		m_dataModel->appendValue(chix, timeline::Sample{dt.msecsSinceEpoch(), v});
	}
	m_dataModel->endAppendValues();
}

#if 0
void MainWindow::paintEvent(QPaintEvent *event)
{
	shvLogFuncFrame();
	QMainWindow::paintEvent(event);
}

void MainWindow::on_action_Open_triggered()
{
	QString fn = QFileDialog::getOpenFileName(this, tr("Open data"), "~/t", tr("Data Files (*.dat)"));
	setDataFile(fn);
}

void MainWindow::setDataFile(const QString &fn)
{
	namespace cp = shv::chainpack;
	if(fn.isEmpty())
		return;
	std::ifstream in(fn.toStdString(), std::ios::binary);
	if (!in.is_open()) {
		shvError() << "failed to open " << fn;
		return;
	}
	while(!in.eof()) {
		bool ok;
		uint64_t chunk_len = cp::ChainPackProtocol::readUIntData(in, &ok);
		if(!ok) {
			shvError() << "reading packet length error";
			return;
		}
		size_t read_len = (size_t)in.tellg() + chunk_len;
		shvInfo() << "\t chunk len:" << chunk_len << "read_len:" << read_len << "stream pos:" << in.tellg();

		uint64_t protocol_version = cp::ChainPackProtocol::readUIntData(in, &ok);
		if(!ok) {
			shvError() << "reading protocol version error";
			return;
		}
		if(protocol_version != 1) {
			shvError() << "Unsupported protocol version";
			return;
		}

		cp::RpcValue val = cp::ChainPackProtocol::read(in);
		shvInfo() << val.toStdString();
	}
}
#endif

void MainWindow::on_action_Open_triggered()
{
	QString qfn = QFileDialog::getOpenFileName(this, tr("Open File"), QString(), tr("Data (*.chainpack *.cpon)"));
	if(!qfn.isEmpty()) {
#ifdef Q_OS_WIN
		std::string fn = qfn.toLocal8Bit().constData();
#else
		std::string fn = qfn.toUtf8().constData();
#endif
		openLogFile(fn);
	}
}

void MainWindow::on_actPause_triggered(bool checked)
{
	m_paused = checked;
}
