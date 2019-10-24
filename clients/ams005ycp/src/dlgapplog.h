#ifndef DLGAPPLOG_H
#define DLGAPPLOG_H

#include <QDialog>

namespace Ui {
class DlgAppLog;
}

class DlgAppLog : public QDialog
{
	Q_OBJECT

public:
	explicit DlgAppLog(QWidget *parent = nullptr);
	~DlgAppLog();
private:
	Ui::DlgAppLog *ui;
};

#endif // DLGAPPLOG_H
