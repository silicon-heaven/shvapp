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
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow() override;
protected:
	void resizeEvent(QResizeEvent *event) override;
private:
#ifdef TESTING
#endif
	//void onVisuWidgetContextMenuRequest(const QPoint &pos);
private:
	void closeEvent(QCloseEvent *event) override;
	bool checkPassword();
private:
	Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
