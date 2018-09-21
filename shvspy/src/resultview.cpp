#include "resultview.h"
#include "ui_resultview.h"

ResultView::ResultView(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::ResultView)
{
	ui->setupUi(this);
}

ResultView::~ResultView()
{
	delete ui;
}

void ResultView::setText(const QString &s)
{
	ui->textBrowser->setPlainText(s);
}
