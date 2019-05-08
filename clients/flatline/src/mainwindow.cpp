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
	m_graph = new timeline::Graph(this);
	m_graph->setModel(m_dataModel);
	m_graphWidget->setGraph(m_graph);

	//timeline::Graph::GraphStyle style;
	//style.setColorBackground(QColor(Qt::darkGray).darker(400));
	//m_graphWidget->setStyle(style);

	FlatLineApp *app = FlatLineApp::instance();
	connect(app->rpcConnection(), &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &MainWindow::onRpcMessageReceived);
	//connect(this, &MainWindow::sendRpcMessage, app->rpcConnection(), &shv::iotqt::rpc::ClientConnection::sendMessage);

	connect(ui->btGenerateSamples, &QPushButton::clicked, this, &MainWindow::generateRandomSamples);
	if(app->cliOptions()->logFile().empty()) {
		QTimer::singleShot(0, this, &MainWindow::generateRandomSamples);
	}
	else {
		openLogFile(app->cliOptions()->logFile());
	}
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::generateRandomSamples()
{
	timeline::GraphModel *model = m_dataModel;

	model->clear();
	int ch_ix = 0;
	model->appendChannel();
	model->setChannelData(ch_ix++, "TC", timeline::GraphModel::ChannelDataRole::Name);
	model->appendChannel();
	model->setChannelData(ch_ix++, "ADC", timeline::GraphModel::ChannelDataRole::Name);
	model->appendChannel();
	model->setChannelData(ch_ix++, "Temp", timeline::GraphModel::ChannelDataRole::Name);
	model->appendChannel();
	model->setChannelData(ch_ix++, "Něco", timeline::GraphModel::ChannelDataRole::Name);
	model->beginAppendValues();
	int cnt = ui->edSamplesCount->value();
	int interval_msec = ui->edTimeInterval->value();
	QRandomGenerator *rnd = QRandomGenerator::global();
	ch_ix = 0;
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
			double v = rnd->generateDouble() * 60 - 30;
			model->appendValue(ch_ix, timeline::Sample{time + t, v});
			time += t;
		}
		ch_ix++;
	}
	/*
	{
		//int64_t time = 0;
		for (int i = 0; i < model->count(ch_ix - 1); ++i) {
			timeline::Sample s = model->sampleAt(ch_ix - 1, i);
			s.value = s.value.toDouble() > 0? true: false;
			model->appendValue(ch_ix, std::move(s));
			//time += t;
		}
		ch_ix++;
	}
	*/
	{
		int64_t time = 0;
		for (int i = 0; i < cnt; ++i) {
			int t = rnd->bounded(interval_msec);
			double v = rnd->generateDouble() * 60 - 30;
			//if((i < 5) || (i >= cnt / 2 && i < cnt / 2 + 20) || (i > cnt -5) )
				model->appendValue(ch_ix, timeline::Sample{time + t, v});
			time += t;
		}
		ch_ix++;
	}
	model->endAppendValues();

	timeline::Graph *gr = m_graphWidget->graph();
	gr->clearChannels();
	gr->setXRange(model->xRange());
	int model_ix = 0;
	{
		timeline::Graph::Channel &ch = gr->appendChannel();
		ch.setModelIndex(model_ix);
		gr->setYRange(gr->channelCount() - 1, model->yRange(ch.modelIndex()));
		timeline::Graph::ChannelStyle ch_style = ch.style();
		ch_style.setInterpolation(timeline::Graph::ChannelStyle::Interpolation::Stepped);
		ch_style.setColor(Qt::cyan);
		ch_style.setHeightMax(6);
		ch.setStyle(ch_style);
		model_ix++;
	}
	{
		timeline::Graph::Channel &ch = gr->appendChannel();
		ch.setModelIndex(model_ix);
		gr->setYRange(gr->channelCount() - 1, model->yRange(ch.modelIndex()));
		timeline::Graph::ChannelStyle ch_style = ch.style();
		ch_style.setColor("orange");
		ch_style.setColorLineArea("orange");
		//ch_style.setColorGrid(QColor());
		//ch_style.setHeightMax(6);
		ch_style.setInterpolation(timeline::Graph::ChannelStyle::Interpolation::None);
		ch_style.setLineAreaStyle(timeline::Graph::ChannelStyle::LineAreaStyle::Filled);
		ch.setStyle(ch_style);
		//ix++;
	}
	{
		timeline::Graph::Channel &ch = gr->appendChannel();
		ch.setModelIndex(model_ix);
		gr->setYRange(gr->channelCount() - 1, model->yRange(ch.modelIndex()));
		timeline::Graph::ChannelStyle ch_style = ch.style();
		ch_style.setLineWidth(0.2);
		ch_style.setInterpolation(timeline::Graph::ChannelStyle::Interpolation::Stepped);
		ch_style.setLineAreaStyle(timeline::Graph::ChannelStyle::LineAreaStyle::Filled);
		ch_style.setColor(Qt::magenta);
		ch_style.setHeightMin(2);
		ch_style.setHeightMax(2);
		ch.setStyle(ch_style);
		gr->enlargeYRange(model_ix, ch_style.lineWidth() / 2);
		model_ix++;
	}
	{
		timeline::Graph::Channel &ch = gr->appendChannel();
		ch.setModelIndex(model_ix);
		gr->setYRange(gr->channelCount() - 1, model->yRange(ch.modelIndex()));
		timeline::Graph::ChannelStyle ch_style = ch.style();
		ch_style.setColor("red");
		//ch_style.setColorLineArea("red");
		//ch_style.setHeightMax(6);
		ch_style.setInterpolation(timeline::Graph::ChannelStyle::Interpolation::Line);
		ch_style.setLineAreaStyle(timeline::Graph::ChannelStyle::LineAreaStyle::Filled);
		ch.setStyle(ch_style);
		model_ix++;
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
		LogDataType type = LogDataType::General;
		std::ifstream file_is;
		file_is.open(fn, std::ios::binary);
		shv::chainpack::AbstractStreamReader *rd;
		if(shv::core::String::endsWith(fn, ".chainpack"))
			rd = new shv::chainpack::ChainPackReader(file_is);
		else if(shv::core::String::endsWith(fn, ".cpon"))
			rd = new shv::chainpack::CponReader(file_is);
		else if(shv::core::String::endsWith(fn, ".brclab")) {
			rd = new shv::chainpack::ChainPackReader(file_is);
			type = LogDataType::BrcLab;
		}
		else
			SHV_EXCEPTION("Unknown extension");
		shv::chainpack::RpcValue rv = rd->read();
		file_is.close();
		delete rd;
		setLogData(rv, type);
	}
	catch (shv::core::Exception &e) {
		shvError() << "Error reading file:" << fn << e.message();
	}
}

void MainWindow::setLogData(const shv::chainpack::RpcValue &data, LogDataType type)
{
	if(m_dataModel)
		delete m_dataModel;
	m_dataModel = new timeline::GraphModel(this);
	m_graph->setModel(m_dataModel);
	m_dataModel->beginAppendValues();
	if(type == LogDataType::General) {
		if(data.isList()) {
			addLogEntries(data.toList());
		}
	}
	else if(type == LogDataType::BrcLab) {
		shvDebug() << data.metaData().toPrettyString();
		if(data.metaData().value("compression").toString() == "qCompress") {
			shvDebug() << "uncompressing started ...";
			const shv::chainpack::RpcValue::String &s = data.toString();
			QByteArray ba = qUncompress((const uchar*)s.data(), (int)s.size());
			shvDebug() << "decoding started ...";
			shv::chainpack::RpcValue body = cp::RpcValue::fromChainPack(ba.toStdString());
			//shvDebug() << body.toPrettyString();
			const shv::chainpack::RpcValue::List &devices = body.toMap().value("devices").toList();
			for (shv::chainpack::RpcValue devicerv : devices) {
				const shv::chainpack::RpcValue::Map &device = devicerv.toMap();
				const cp::RpcValue::Map &scanning = device.value("scanning").toMap();
				const cp::RpcValue::List &scanning_series = scanning.value("series").toList();
				static const char *data_type_names[] = {
					"Amp",
					"Cos",
					"Sin",
					"P3",
					"P4",
					"Osc",
					"BlockedAmp",
					"BlockedCos",
					"BlockedSin",
					"BlockedP3",
					"BlockedP4",
					"BlockedOsc",
					"DataMissing",
					"VehicleEvent",
					"ScanningVehicleType",
					"ComputedBlockedAmp",
					"ComputedBlockedCos",
					"ComputedBlockedSin",
					"ComputedBlockedP3",
					"ComputedBlockedP4",
					"ComputedBlockedOsc",
				};
				constexpr int types_cnt = sizeof (data_type_names) / sizeof (char*);
				int data_type = 0;
				for (const cp::RpcValue &serie_data : scanning_series) {
					if (data_type >= types_cnt) {
						shvError() << "Invalid chainpack format, too much series";
						break;
					}
					m_dataModel->appendChannel();
					QString name = data_type_names[data_type];
					m_dataModel->setChannelData(m_dataModel->channelCount() - 1, name, timeline::GraphModel::ChannelDataRole::Name);
					const cp::RpcValue::Map &serie = serie_data.toMap();
					const cp::RpcValue::List &value_x = serie.value("x").toList();
					const cp::RpcValue::List &value_y = serie.value("y").toList();
					if (value_x.size() != value_y.size()) {
						shvError() << "x-y values have to have same count";
						break;
					}
					for (size_t i = 0; i < value_x.size(); ++i) {
						timeline::Sample sample;
						sample.time = value_x[i].toDateTime().msecsSinceEpoch();
						sample.value = shv::iotqt::Utils::rpcValueToQVariant(value_y[i]);
						m_dataModel->appendValue(m_dataModel->channelCount() - 1, std::move(sample));
					}
					++data_type;
				}
			}
		}
	}
	m_dataModel->endAppendValues();
	m_graph->createChannelsFromModel();
	m_graph->makeLayout(ui->graphView->widget()->rect());
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
