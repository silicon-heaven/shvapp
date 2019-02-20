#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <shv/coreqt/utils.h>

#include <QMainWindow>

class QPoint;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

	using Super = QMainWindow;
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private:
#ifdef TESTING
	Q_SLOT void on_ompagOn_toggled(bool on);
	Q_SLOT void on_convOn_toggled(bool on);
	Q_SLOT void on_pwrOn_toggled(bool on);
#endif
	//void onVisuWidgetContextMenuRequest(const QPoint &pos);
private:
	void closeEvent(QCloseEvent *event) override;
private:
	Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
