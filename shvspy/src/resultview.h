#pragma once

#include <QDialog>

namespace Ui {
class ResultView;
}

class ResultView : public QDialog
{
	Q_OBJECT

public:
	explicit ResultView(QWidget *parent = 0);
	~ResultView();

	void setText(const QString &s);

private:
	Ui::ResultView *ui;
};
