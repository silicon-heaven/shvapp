#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}
class QCloseEvent;
class QStandardItemModel;

class ShvBrokerNodeItem;

class MainWindow : public QMainWindow
{
	Q_OBJECT
private:
	typedef QMainWindow Super;
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();
protected:
	Q_SLOT void on_actAddServer_triggered();
	Q_SLOT void on_actEditServer_triggered();
	Q_SLOT void on_actCopyServer_triggered();
	Q_SLOT void on_actRemoveServer_triggered();
	//Q_SLOT void on_actSubscribeNodeValue_triggered();

	Q_SLOT void on_treeServers_doubleClicked(const QModelIndex &ix) {openNode(ix);}
	Q_SLOT void on_treeServers_enterKeyPressed(const QModelIndex &ix) {openNode(ix);}
	Q_SLOT void on_treeServers_customContextMenuRequested(const QPoint &pos);
	Q_SLOT void onShvTreeViewCurrentSelectionChanged(const QModelIndex &curr_ix, const QModelIndex &prev_ix);

	void editServer(ShvBrokerNodeItem *srv, bool copy_server);
	void openNode(const QModelIndex &ix);
	void displayResult(const QModelIndex &ix);
	void editMethodParameters(const QModelIndex &ix);
	void attributesTableContexMenu(const QPoint &point);

	void closeEvent(QCloseEvent *ev) Q_DECL_OVERRIDE;
private:
	Ui::MainWindow *ui;
	QStandardItemModel *m_opcObjects;
};

#endif // MAINWINDOW_H
