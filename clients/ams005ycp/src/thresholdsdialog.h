#ifndef THRESHOLDSDIALOG_H
#define THRESHOLDSDIALOG_H

#include <QDialog>

namespace Ui {
class ThresholdsDialog;
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
	void saveTresholdIfEdited(const std::string &shv_name, QSpinBox *editor);
private:
	Ui::ThresholdsDialog *ui;

	std::map<std::string, int> m_origTresholds;
};

#endif // THRESHOLDSDIALOG_H
