#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <shv/chainpack/rpcvalue.h>

#include <QMainWindow>

namespace shv { namespace chainpack { class RpcMessage; }}
namespace timeline { class GraphWidget; class GraphModel; class Graph; }

namespace Ui {
class MainWindow;
}

struct DataSample;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;

private slots:
	void on_action_Open_triggered();
	void on_actPause_triggered(bool checked);

private:
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void openLogFile(const std::string &fn);
	void setLogData(const shv::chainpack::RpcValue &data);
private:
	//void on_action_Open_triggered();
	void addLogEntries(const shv::chainpack::RpcValue::List &data);
protected:
	//void paintEvent(QPaintEvent *event) override;
private:
	void generateRandomSamples();
private:
	Ui::MainWindow *ui;

	timeline::Graph *m_graph = nullptr;
	timeline::GraphWidget *m_graphWidget = nullptr;
	timeline::GraphModel *m_dataModel = nullptr;
	bool m_paused = false;
};

#endif // MAINWINDOW_H
