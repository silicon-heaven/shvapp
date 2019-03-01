#ifndef THRESHOLDSDIALOG_H
#define THRESHOLDSDIALOG_H

#include <QDialog>

namespace Ui {
class TresholdsDialog;
}

class QSpinBox;

class ThresholdsDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ThresholdsDialog(QWidget *parent = nullptr);
	~ThresholdsDialog();
private:
	void loadTreshold(const std::string &shv_name, QSpinBox *editor);
private:
	Ui::TresholdsDialog *ui;
};

#endif // THRESHOLDSDIALOG_H
