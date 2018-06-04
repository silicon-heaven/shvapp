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

	SHV_PROPERTY_IMPL2(int, o, O, mpagStatus, 0)
	SHV_PROPERTY_IMPL2(int, b, B, sStatus, 0)

public:
	enum class SwitchStatus {Unknown = 0, Off, On};
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void on_actionOmpagOn_toggled(bool on);
	void on_actionBsOn_toggled(bool on);

	void refreshStatus();

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
