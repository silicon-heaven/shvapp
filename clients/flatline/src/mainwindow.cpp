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
	ui->graphView->setWidget(new timeline::GraphWidget());
	shvInfo() << qobject_cast<QWidget*>(ui->graphView->widget());
	{
		// scan tab
		/*
		QColor grid_color(Qt::darkGreen);
		QColor label_color(grid_color.lighter(200));
		ui->graphView->settings.xAxis.description.text = tr("Time [MSec]");
		ui->graphView->settings.xAxis.rangeType = ui->graphView->settings.xAxis.Fixed;
		ui->graphView->settings.xAxisType = shv::coreqt::data::ValueType::Int;
		//ui->graphView->settings.xAxis.rangeMin = 15000;
		//ui->graphView->settings.xAxis.rangeMax = 40000;
		ui->graphView->settings.xAxis.color = label_color;

		ui->graphView->settings.yAxis.description.text = tr("Value [bits]");
		ui->graphView->settings.yAxis.rangeMin = -2048;
		ui->graphView->settings.yAxis.rangeMax = 2048;
		ui->graphView->settings.yAxis.color = label_color;

		ui->graphView->settings.y2Axis.description.text = tr("Time 16 bit");
		ui->graphView->settings.y2Axis.rangeMin = 0;
		ui->graphView->settings.y2Axis.rangeMax = 65535;
		ui->graphView->settings.y2Axis.show = true;
		ui->graphView->settings.y2Axis.color = QColor("orange");//.lighter();

		new shv::gui::graphview::IntSerie(DataSample::Voltage, "Voltage", DataSample::channelColor(DataSample::Voltage), ui->graphView);
		new shv::gui::graphview::IntSerie(DataSample::Current, "Current", DataSample::channelColor(DataSample::Current), ui->graphView);
		new shv::gui::graphview::IntSerie(DataSample::Power, "Power", DataSample::channelColor(DataSample::Power), ui->graphView);
		auto *time_serie = new shv::gui::graphview::IntSerie(DataSample::Power + 1, "Raw time", DataSample::channelColor(DataSample::Power+1), ui->graphView);
		time_serie->setRelatedAxis(shv::gui::graphview::Serie::YAxis::Y2);

		ui->graphView->settings.verticalGrid.fixedCount = 3;
		ui->graphView->settings.verticalGrid.color = grid_color;
		ui->graphView->settings.horizontalGrid.fixedCount = 3;
		ui->graphView->settings.horizontalGrid.color = grid_color;
		ui->graphView->settings.rangeSelector.show = true;
		ui->graphView->settings.backgroundColor = QColor(Qt::darkGray).darker(400);

		//ui->graphView->settings.legend;
		namespace sd = shv::coreqt::data;
		m_scanSamples = new shv::gui::GraphModelData(this);
		m_scanSamples->addSerie(sd::SerieData(sd::ValueType::Int, sd::ValueType::Int));
		m_scanSamples->addSerie(sd::SerieData(sd::ValueType::Int, sd::ValueType::Int));
		m_scanSamples->addSerie(sd::SerieData(sd::ValueType::Int, sd::ValueType::Int));
		m_scanSamples->addSerie(sd::SerieData(sd::ValueType::Int, sd::ValueType::Int));
		auto *m = new shv::gui::GraphModel(m_scanSamples, this);
		ui->graphView->setModel(m);
	*/
	}

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
	timeline::GraphView *view = ui->graphView;
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
	int64_t time = 0;
	QRandomGenerator *rnd = QRandomGenerator::global();
	for (int i = 0; i < cnt; ++i) {
		int t = rnd->bounded(interval_msec);
		int v = rnd->bounded(2);
		model->appendValue(0, timeline::GraphModel::Sample{time + t, v});
		time += t;
	}
	time = 0;
	for (int i = 0; i < cnt; ++i) {
		int t = rnd->bounded(interval_msec);
		int v = rnd->bounded(-2048, 2048);
		model->appendValue(1, timeline::GraphModel::Sample{time + t, v});
		time += t;
	}
	time = 0;
	for (int i = 0; i < cnt; ++i) {
		int t = rnd->bounded(interval_msec);
		double v = rnd->generateDouble() * 60 - 30;
		model->appendValue(2, timeline::GraphModel::Sample{time + t, v});
		time += t;
	}
	model->endAppendValues();

	view->clearChannels();
	{
		view->appendChannel();
		timeline::GraphView::Channel &ch = view->channelAt(0);
		ch.style.setColor(Qt::magenta);
		ch.style.setHeightMin(1);
		ch.style.setHeightMax(1);
	}
	{
		view->appendChannel();
		timeline::GraphView::Channel &ch = view->channelAt(1);
		ch.style.setColor(Qt::cyan);
		ch.style.setHeightMax(6);
	}
	{
		view->appendChannel();
		timeline::GraphView::Channel &ch = view->channelAt(2);
		ch.style.setColor(Qt::yellow);
		//ch.style.setHeightMax(6);
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
	ui->graphView->setModel(m_dataModel);
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
		m_dataModel->appendValue(chix, timeline::GraphModel::Sample{dt.msecsSinceEpoch(), v});
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
