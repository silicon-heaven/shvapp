#pragma once

#include <QDialog>

namespace Ui {
class ResultView;
}

class ResultView : public QDialog
{
	Q_OBJECT

	using Super = QDialog;
public:
	explicit ResultView(QWidget *parent = nullptr);
	~ResultView() override;

	void setText(const QString &s);
public slots:
	//int exec() override;
private:
	//void restoreGeometry();
private:
	Ui::ResultView *ui;
};
