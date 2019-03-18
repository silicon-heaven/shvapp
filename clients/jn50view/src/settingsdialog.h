#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog
{
	Q_OBJECT

	using Super = QDialog;
public:
	explicit SettingsDialog(QWidget *parent = nullptr);
	~SettingsDialog() override;
	/*
	QString switchName();
	QString fileName();
	int checkInterval();
	*/
private:
	Ui::SettingsDialog *ui;

public slots:
	void done(int status) override;
};

#endif // SETTINGSDIALOG_H
