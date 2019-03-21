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
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;
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

	void resizeAttributesViewSectionsToFit();

	void editServer(ShvBrokerNodeItem *srv, bool copy_server);
	void openNode(const QModelIndex &ix);
	void displayResult(const QModelIndex &ix);
	void editMethodParameters(const QModelIndex &ix);
	void editStringParameter(const QModelIndex &ix);
	void editCponParameters(const QModelIndex &ix);

	void onAttributesTableContexMenu(const QPoint &point);

	void closeEvent(QCloseEvent *ev) Q_DECL_OVERRIDE;
	void saveSettings();
private:
	Ui::MainWindow *ui;
	QStandardItemModel *m_opcObjects;
};

#endif // MAINWINDOW_H
