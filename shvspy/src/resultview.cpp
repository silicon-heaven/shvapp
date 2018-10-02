#include "resultview.h"
#include "ui_resultview.h"

#include <QSettings>

ResultView::ResultView(QWidget *parent)
	: Super(parent)
	, ui(new Ui::ResultView)
{
	ui->setupUi(this);

	QSettings settings;
	restoreGeometry(settings.value(QStringLiteral("ui/ResultView/geometry")).toByteArray());
}

ResultView::~ResultView()
{
	QSettings settings;
	settings.setValue(QStringLiteral("ui/ResultView/geometry"), saveGeometry());
	delete ui;
}

void ResultView::setText(const QString &s)
{
	ui->textBrowser->setPlainText(s);
}


