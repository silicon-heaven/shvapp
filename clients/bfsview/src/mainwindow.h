#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <shv/coreqt/utils.h>

#include <QMainWindow>
#include <QDomDocument>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void on_actionOmpagOn_toggled(bool on);
	void on_actionBsOn_toggled(bool on);

	void refreshStatus();

	void on_actionPwrStatus_toggled(bool on);

private:
	QDomElement elementById(const QString &id);
	void setElementFillColor(const QString &id, const QString &c);
	void setElementVisible(const QString &elem_id, bool on);
	void setElementStyleAttribute(const QString &elem_id, const QString &key, const QString &val);
private:
	Ui::MainWindow *ui;
	QDomDocument m_xDoc;
};

#endif // MAINWINDOW_H
