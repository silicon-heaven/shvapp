#pragma once

#include <QWidget>

namespace Ui {
class LastUsedParamsWidget;
}

class LastUsedParamsWidget : public QWidget
{
	Q_OBJECT

public:
	explicit LastUsedParamsWidget(const QString &path, const QString &method, QWidget *parent = 0);
	~LastUsedParamsWidget();

	Q_SIGNAL void paramSelected(const QString &param);

private:
	Ui::LastUsedParamsWidget *ui;
};
