#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "flatlineapp.h"
#include "appclioptions.h"

#include <shv/chainpack/chainpackreader.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/valuechange.h>
#include <shv/coreqt/log.h>
#include <shv/iotqt/utils.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/core/string.h>
#include <shv/core/stringview.h>
#include <shv/iotqt/rpc/clientconnection.h>
#include <shv/visu/timeline/graphmodel.h>
#include <shv/visu/timeline/graphwidget.h>
#include <shv/core/utils/shvfilejournal.h>
#include <shv/core/utils/shvlogfilereader.h>

#include <QFileDialog>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QTimer>

#include <fstream>
//#include <iostream>

namespace cp = shv::chainpack;
namespace tl = shv::visu::timeline;

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	FlatLineApp *app = FlatLineApp::instance();

	ui->setupUi(this);

	connect(ui->actHelpAbout, &QAction::triggered, [this]() {
		QMessageBox::about(this
						   , tr("Flat Line")
						   , tr("<p><b>Flat Line</b></p>"
							 "<p>ver. %1</p>"
							 "<p>2019 Elektroline a.s.</p>"
							 "<p><a href=\"www.elektroline.cz\">www.elektroline.cz</a></p>")
						   .arg(QCoreApplication::applicationVersion())
						   );
	});
	ui->frmTest->hide();
	connect(ui->actViewTest, &QAction::toggled, [this](bool is_checked) {
		ui->frmTest->setVisible(is_checked);
	});
	ui->actViewTest->setChecked(app->cliOptions()->isTestMode());
	connect(ui->edTimeInterval, QOverload<int>::of(&QSpinBox::valueChanged), [this]() {
		runLiveSamples(ui->btLiveSamples->isChecked());
	});

	m_dataModel = new tl::GraphModel(this);
	connect(m_dataModel, &tl::GraphModel::xRangeChanged, this, &MainWindow::onGraphXRangeChanged);
	//ui->graphView->viewport()->show();
	m_graphWidget = new tl::GraphWidget();

	ui->graphView->setBackgroundRole(QPalette::Dark);
	ui->graphView->setWidget(m_graphWidget);
	ui->graphView->widget()->setBackgroundRole(QPalette::ToolTipBase);
	ui->graphView->widget()->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);

	//shvInfo() << qobject_cast<QWidget*>(ui->graphView->widget());
	m_graph = new tl::Graph(this);
	m_graph->setModel(m_dataModel);
	m_graphWidget->setGraph(m_graph);

	connect(m_dataModel, &tl::GraphModel::channelCountChanged, [this]() {
		m_graph->createChannelsFromModel();
		ui->graphView->makeLayout();
	});
	connect(m_dataModel, &tl::GraphModel::xRangeChanged, [this](tl::XRange range) {
		m_graph->setXRange(range);
		m_graphWidget->update();
	});

	//tl::Graph::GraphStyle style;
	//style.setColorBackground(QColor(Qt::darkGray).darker(400));
	//m_graphWidget->setStyle(style);

	connect(app->rpcConnection(), &shv::iotqt::rpc::ClientConnection::rpcMessageReceived, this, &MainWindow::onRpcMessageReceived);
	//connect(this, &MainWindow::sendRpcMessage, app->rpcConnection(), &shv::iotqt::rpc::ClientConnection::sendMessage);

	connect(ui->btGenerateSamples, &QPushButton::clicked, this, &MainWindow::generateRandomSamples);
	connect(ui->btLiveSamples, &QPushButton::toggled, this, &MainWindow::runLiveSamples);
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
	tl::GraphModel *model = m_dataModel;

	model->clear();
	int ch_ix = 0;
	model->appendChannel();
	model->setChannelData(ch_ix, "TC", tl::GraphModel::ChannelDataRole::ShvPath);
	model->setChannelData(ch_ix, model->channelData(ch_ix, tl::GraphModel::ChannelDataRole::ShvPath), tl::GraphModel::ChannelDataRole::Name);
	ch_ix++;
	model->appendChannel();
	model->setChannelData(ch_ix, "ADC", tl::GraphModel::ChannelDataRole::ShvPath);
	model->setChannelData(ch_ix, model->channelData(ch_ix, tl::GraphModel::ChannelDataRole::ShvPath), tl::GraphModel::ChannelDataRole::Name);
	ch_ix++;
	model->appendChannel();
	model->setChannelData(ch_ix, "Temp", tl::GraphModel::ChannelDataRole::ShvPath);
	model->setChannelData(ch_ix, model->channelData(ch_ix, tl::GraphModel::ChannelDataRole::ShvPath), tl::GraphModel::ChannelDataRole::Name);
	ch_ix++;
	model->appendChannel();
	model->setChannelData(ch_ix, "Něco", tl::GraphModel::ChannelDataRole::ShvPath);
	model->setChannelData(ch_ix, model->channelData(ch_ix, tl::GraphModel::ChannelDataRole::ShvPath), tl::GraphModel::ChannelDataRole::Name);
	ch_ix++;
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
			model->appendValue(ch_ix, tl::Sample{time + t, v});
			time += t;
		}
		ch_ix++;
	}
	{
		int64_t time = 0;
		for (int i = 0; i < cnt; ++i) {
			int t = rnd->bounded(interval_msec);
			double v = rnd->generateDouble() * 60 - 30;
			model->appendValue(ch_ix, tl::Sample{time + t, v});
			time += t;
		}
		ch_ix++;
	}
	/*
	{
		//int64_t time = 0;
		for (int i = 0; i < model->count(ch_ix - 1); ++i) {
			tl::Sample s = model->sampleAt(ch_ix - 1, i);
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
				model->appendValue(ch_ix, tl::Sample{time + t, v});
			time += t;
		}
		ch_ix++;
	}
	model->endAppendValues();

	tl::Graph *gr = m_graphWidget->graph();
	gr->clearChannels();
	gr->setXRange(model->xRange());
	int model_ix = 0;
	{
		tl::Graph::Channel &ch = gr->appendChannel();
		ch.setModelIndex(model_ix);
		gr->setYRange(gr->channelCount() - 1, model->yRange(ch.modelIndex()));
		tl::Graph::ChannelStyle ch_style = ch.style();
		ch_style.setInterpolation(tl::Graph::ChannelStyle::Interpolation::Stepped);
		ch_style.setColor(Qt::cyan);
		ch_style.setHeightMax(6);
		ch.setStyle(ch_style);
		model_ix++;
	}
	{
		tl::Graph::Channel &ch = gr->appendChannel();
		ch.setModelIndex(model_ix);
		gr->setYRange(gr->channelCount() - 1, model->yRange(ch.modelIndex()));
		tl::Graph::ChannelStyle ch_style = ch.style();
		ch_style.setColor("orange");
		ch_style.setColorLineArea("orange");
		//ch_style.setColorGrid(QColor());
		//ch_style.setHeightMax(6);
		ch_style.setInterpolation(tl::Graph::ChannelStyle::Interpolation::None);
		ch_style.setLineAreaStyle(tl::Graph::ChannelStyle::LineAreaStyle::Filled);
		ch.setStyle(ch_style);
		//ix++;
	}
	{
		tl::Graph::Channel &ch = gr->appendChannel();
		ch.setModelIndex(model_ix);
		gr->setYRange(gr->channelCount() - 1, model->yRange(ch.modelIndex()));
		tl::Graph::ChannelStyle ch_style = ch.style();
		ch_style.setLineWidth(0.2);
		ch_style.setInterpolation(tl::Graph::ChannelStyle::Interpolation::Stepped);
		ch_style.setLineAreaStyle(tl::Graph::ChannelStyle::LineAreaStyle::Filled);
		ch_style.setColor(Qt::magenta);
		ch_style.setHeightMin(2);
		ch_style.setHeightMax(2);
		ch.setStyle(ch_style);
		gr->enlargeYRange(model_ix, ch_style.lineWidth() / 2);
		model_ix++;
	}
	{
		tl::Graph::Channel &ch = gr->appendChannel();
		ch.setModelIndex(model_ix);
		gr->setYRange(gr->channelCount() - 1, model->yRange(ch.modelIndex()));
		tl::Graph::ChannelStyle ch_style = ch.style();
		ch_style.setColor("red");
		//ch_style.setColorLineArea("red");
		//ch_style.setHeightMax(6);
		ch_style.setInterpolation(tl::Graph::ChannelStyle::Interpolation::Line);
		ch_style.setLineAreaStyle(tl::Graph::ChannelStyle::LineAreaStyle::Filled);
		ch.setStyle(ch_style);
		model_ix++;
	}

	ui->graphView->makeLayout();
}

void MainWindow::runLiveSamples(bool on)
{
	if(on) {
		if(!m_liveSamplesTimer) {
			m_liveSamplesTimer = new QTimer(this);
			connect(m_liveSamplesTimer, &QTimer::timeout, [this]() {
				tl::Graph *gr = m_graphWidget->graph();
				QRandomGenerator *rnd = QRandomGenerator::global();
				tl::XRange xrange = m_dataModel->xRange();
				int64_t msec = xrange.isValid()? xrange.max: 0;
				msec += rnd->bounded(ui->edTimeInterval->value());
				m_dataModel->beginAppendValues();
				if(ui->edExtraChannelsCount->value() > 0) {
					int ix = rnd->bounded(ui->edExtraChannelsCount->value());
					cp::RpcValue::List entry;
					entry.push_back(cp::RpcValue::DateTime::fromMSecsSinceEpoch(msec));
					std::string shv_path = QString("Cha_%1").arg(ix+1).toStdString();
					entry.push_back(shv_path);
					double val = rnd->bounded((ix+1) * 100) - ((ix+1) * 100/2);
					entry.push_back(val);
					shvDebug() << shv_path << "->" << cp::RpcValue(entry).toCpon();
					addLogEntry(entry);
				}
				else {
					int ch_ix = rnd->bounded(gr->channelCount());
					const tl::Graph::Channel &ch = gr->channelAt(ch_ix);
					const auto yrange = ch.yRange();
					double val = rnd->bounded(yrange.interval()) + yrange.min;
					cp::RpcValue::List entry;
					entry.push_back(cp::RpcValue::DateTime::fromMSecsSinceEpoch(msec));
					int m_ix = ch.modelIndex();
					std::string shv_path = m_dataModel->channelData(m_ix, tl::GraphModel::ChannelDataRole::ShvPath).toString().toStdString();
					entry.push_back(shv_path);
					entry.push_back(val);
					shvDebug() << shv_path << "->" << cp::RpcValue(entry).toCpon();
					addLogEntry(entry);
				}
				m_dataModel->endAppendValues();
			});
		}
		m_liveSamplesTimer->start(ui->edTimeInterval->value());
	}
	else {
		SHV_SAFE_DELETE(m_liveSamplesTimer);
	}
}

void MainWindow::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	shvDebug() << msg.toCpon();
	if(msg.isSignal()) {
		cp::RpcSignal ntf(msg);
		shvDebug() << "SIG:" << ntf.toCpon();
		if(shv::core::String::endsWith(ntf.shvPath().toString(), "/data") && ntf.method() == cp::Rpc::SIG_VAL_CHANGED) {
			if(!m_paused) {
				/*
				const cp::RpcValue::List& data = ntf.params().toList();
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
	shvLogFuncFrame() << fn;
	try {
		LogDataType type = LogDataType::General;
		shv::chainpack::RpcValue rv;
		std::ifstream file_is;
		file_is.open(fn, std::ios::binary);
		shv::chainpack::AbstractStreamReader *rd = nullptr;
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
		if(rd) {
			rv = rd->read();
			delete rd;
			file_is.close();
		}
		setLogData(rv, type);
	}
	catch (shv::core::Exception &e) {
		shvError() << "Error reading file:" << fn << e.message();
	}
}

void MainWindow::setLogData(const shv::chainpack::RpcValue &data, LogDataType type)
{
	m_logData = data;
	m_pathsDict.clear();
	m_shortTimePrev = 0;
	m_msecTime = 0;

	m_dataModel->clear();
	m_dataModel->beginAppendValues();
	if(type == LogDataType::General) {
		m_pathsDict = data.metaValue("pathsDict").toIMap();
		addLogEntries(data.toList());
	}
	else if(type == LogDataType::BrcLab) {
		shvDebug() << data.metaData().toPrettyString();
		if(data.metaData().value("compression").toString() == "qCompress") {
			shvDebug() << "uncompressing started ...";
			const shv::chainpack::RpcValue::String &s = data.asString();
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
					m_dataModel->setChannelData(m_dataModel->channelCount() - 1, name, tl::GraphModel::ChannelDataRole::Name);
					const cp::RpcValue::Map &serie = serie_data.toMap();
					const cp::RpcValue::List &value_x = serie.value("x").toList();
					const cp::RpcValue::List &value_y = serie.value("y").toList();
					if (value_x.size() != value_y.size()) {
						shvError() << "x-y values have to have same count";
						break;
					}
					for (size_t i = 0; i < value_x.size(); ++i) {
						tl::Sample sample;
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
	for(const cp::RpcValue &entry : data) {
		addLogEntry(entry);
	}
}

void MainWindow::addLogEntry(const shv::chainpack::RpcValue &entry)
{
	const cp::RpcValue::List &row = entry.toList();

	cp::RpcValue::DateTime dt = row.value(0).toDateTime();
	cp::RpcValue rv_path = row.value(1);
	if(rv_path.isUInt() || rv_path.isInt())
		rv_path = m_pathsDict.value(rv_path.toInt());
	const shv::chainpack::RpcValue::String &path = rv_path.asString();
	if(path.empty()) {
		shvError() << "invalid entry path:" << entry.toCpon();
		return;
	}
	cp::RpcValue val = row.value(2);
	cp::RpcValue short_time = row.value(3);

	//if(m_msecTime == 0)
	//	m_msecTime = dt.msecsSinceEpoch();
	/*
	if(m_deviceType == DeviceType::Andi) {
		if(val.isList()) {
			unsigned short_time = val.toList().value(1).toUInt();
			int64_t msec = convertShortTime(short_time);
			val = val.toList().value(0);
			appendModelValue(path, msec, val);
		}
	}
	else if(m_deviceType == DeviceType::Anca) {
		if(path == "data") {
			if(val.isList()) {
				const shv::chainpack::RpcValue::List &rec = val.toList();
				unsigned short_time = rec.value(0).toUInt();
				int64_t msec = convertShortTime(short_time);
				cp::RpcValue volt = rec.value(1);
				cp::RpcValue curr = rec.value(2);
				cp::RpcValue pow = rec.value(3);
				appendModelValue("voltage", msec, volt);
				appendModelValue("current", msec, curr);
				appendModelValue("power", msec, pow);
			}
		}
	}
	else {
		int64_t msec = convertShortTime(short_time);
		appendModelValue(path, dt.msecsSinceEpoch(), val);
	}
	*/
	int64_t msec = dt.msecsSinceEpoch();
	if(short_time.isUInt()) {
		int64_t msec2 = convertShortTime(short_time.toUInt());
		if(msec2 < msec) {
			m_msecTime = msec;
		}
		else {
			msec = msec2;
		}

	}
	appendModelValue(path, msec, val);
}

void MainWindow::appendModelValue(const std::string &path, int64_t msec, const shv::chainpack::RpcValue &rv)
{
	QVariant v = shv::iotqt::Utils::rpcValueToQVariant(rv);
	if(v.isValid())
		m_dataModel->appendValueShvPath(path, tl::Sample{msec, v});
}

int64_t MainWindow::convertShortTime(unsigned short_time)
{
	if(short_time < m_shortTimePrev) {
		m_msecTime += 65536;
	}
	m_shortTimePrev = short_time;
	return m_msecTime + short_time;
}

void MainWindow::onGraphXRangeChanged(const tl::XRange &range)
{
	shvDebug() << "from:" << range.min << "to:" << range.max;
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

void MainWindow::on_acOpenFile_triggered()
{
	QString qfn = QFileDialog::getOpenFileName(this, tr("Open log file"), QString(), tr("Data (*.chainpack *.cpon *.brclab)"));
	if(!qfn.isEmpty()) {
#ifdef Q_OS_WIN
		std::string fn = qfn.toLocal8Bit().constData();
#else
		std::string fn = qfn.toUtf8().constData();
#endif
		openLogFile(fn);
	}
}

void MainWindow::on_actOpenRawFiles_triggered()
{
	QStringList qfns = QFileDialog::getOpenFileNames(this, tr("Open raw log files"), QString(), tr("Raw log (*.log2)"));
	cp::RpcValue::List log;
	for(const QString &qfn : qfns) {
#ifdef Q_OS_WIN
		std::string fn = qfn.toLocal8Bit().constData();
#else
		std::string fn = qfn.toUtf8().constData();
#endif
		{
			shv::core::utils::ShvLogFileReader rd(fn);
			while (rd.next()) {
				const shv::core::utils::ShvJournalEntry &e = rd.entry();
				if(!e.isValid())
					break;
				cp::RpcValue::List rec;
				rec.push_back(shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(e.epochMsec));
				rec.push_back(e.path);
				rec.push_back(e.value);
				if(e.shortTime >= 0)
					rec.push_back(e.shortTime);
				log.push_back(rec);
			}
		}
	}
	shv::chainpack::RpcValue rv;
	rv = log;
	{
			cp::RpcValue::List fields;
			fields.push_back(cp::RpcValue::Map{{shv::core::utils::ShvFileJournal::KEY_NAME, "Timestamp"}});
			fields.push_back(cp::RpcValue::Map{{shv::core::utils::ShvFileJournal::KEY_NAME, "Path"}});
			fields.push_back(cp::RpcValue::Map{{shv::core::utils::ShvFileJournal::KEY_NAME, "Value"}});
			fields.push_back(cp::RpcValue::Map{{shv::core::utils::ShvFileJournal::KEY_NAME, "ShortTime"}});
			rv.setMetaValue("fields", std::move(fields));
		}
	setLogData(rv, LogDataType::General);
}

void MainWindow::on_actPause_triggered(bool checked)
{
	m_paused = checked;
}


void MainWindow::on_actFileSaveAs_triggered()
{
	QString qfn = QFileDialog::getSaveFileName(this, tr("Save log file"),
							   "untitled.chainpack",
							   tr("Images (*.cpon *.chainpack)"));
	if(!qfn.isEmpty()) {
#ifdef Q_OS_WIN
		std::string fn = qfn.toLocal8Bit().constData();
#else
		std::string fn = qfn.toUtf8().constData();
#endif
		std::ofstream os;
		os.open(fn, std::ios::binary);
		shv::chainpack::AbstractStreamWriter *wr = nullptr;
		if(shv::core::String::endsWith(fn, ".chainpack"))
			wr = new shv::chainpack::ChainPackWriter(os);
		else if(shv::core::String::endsWith(fn, ".cpon")) {
			shv::chainpack::CponWriterOptions opts;
			opts.setIndent("\t");
			wr = new shv::chainpack::CponWriter(os, opts);
		}
		else
			SHV_EXCEPTION("Unknown extension");
		if(wr) {
			wr->write(m_logData);
			delete wr;
			os.close();
		}
	}
}
