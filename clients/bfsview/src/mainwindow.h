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

	using Super = QMainWindow;
public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private slots:
	void on_ompagOn_toggled(bool on);
	void on_convOn_toggled(bool on);
	void on_pwrOn_toggled(bool on);

	void refreshStatus();
private:
	QDomElement elementById(const QString &id);
	void setElementText(const QString &elem_id, const QString &text);
	void setElementColor(const QString &id, const QString &c);
	void setElementFillColor(const QString &id, const QString &c);
	void setElementVisible(const QString &elem_id, bool on);
	void setElementStyleAttribute(const QString &elem_id, const QString &key, const QString &val);

	void closeEvent(QCloseEvent *event) override;
private:
	Ui::MainWindow *ui;
	QDomDocument m_xDoc;
};

#endif // MAINWINDOW_H
