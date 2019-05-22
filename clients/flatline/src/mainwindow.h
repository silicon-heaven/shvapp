#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "timeline/sample.h"

#include <shv/chainpack/rpcvalue.h>

#include <QMainWindow>

namespace shv { namespace chainpack { class RpcMessage; }}
namespace timeline { class GraphWidget; class GraphModel; class Graph;}

namespace Ui {
class MainWindow;
}

struct DataSample;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	enum class LogDataType {General, BrcLab};
	enum class DeviceType {General, Andi, Anca};
public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;

private slots:
	void on_action_Open_triggered();
	void on_actPause_triggered(bool checked);

private:
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void openLogFile(const std::string &fn);
	void setLogData(const shv::chainpack::RpcValue &data, LogDataType type);
private:
	//void on_action_Open_triggered();
	void addLogEntries(const shv::chainpack::RpcValue::List &data);
	void addLogEntry(const shv::chainpack::RpcValue &entry);
	void appendModelValue(const std::string &path, int64_t msec, const shv::chainpack::RpcValue &rv);
	int pathToModelIndex(const std::string &path);
	int64_t convertShortTime(unsigned short_time);
private:
	void onGraphXRangeChanged(const timeline::XRange &range);

	void generateRandomSamples();
	void runLiveSamples(bool on);
private:
	Ui::MainWindow *ui;

	timeline::Graph *m_graph = nullptr;
	timeline::GraphWidget *m_graphWidget = nullptr;
	timeline::GraphModel *m_dataModel = nullptr;
	bool m_paused = false;

	std::map<std::string, int> m_pathToModelIndex;
	shv::chainpack::RpcValue::IMap m_pathsDict;

	QTimer *m_liveSamplesTimer = nullptr;

	DeviceType m_deviceType = DeviceType::General;
	unsigned m_shortTimePrev = 0;
	int64_t m_msecTime = 0;
};

#endif // MAINWINDOW_H
