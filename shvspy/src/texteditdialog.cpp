#include "texteditdialog.h"
#include "ui_texteditdialog.h"

#include <QSettings>
#include <QPushButton>

TextEditDialog::TextEditDialog(QWidget *parent)
	: Super(parent)
	, ui(new Ui::TextEditDialog)
{
	ui->setupUi(this);

	QSettings settings;
	restoreGeometry(settings.value(QStringLiteral("ui/ResultView/geometry")).toByteArray());
}

TextEditDialog::~TextEditDialog()
{
	QSettings settings;
	settings.setValue(QStringLiteral("ui/ResultView/geometry"), saveGeometry());
	delete ui;
}

void TextEditDialog::setText(const QString &s)
{
	ui->plainTextEdit->setPlainText(s);
}

QString TextEditDialog::text() const
{
	return ui->plainTextEdit->toPlainText();
}

void TextEditDialog::setReadOnly(bool ro)
{
	ui->plainTextEdit->setReadOnly(ro);
	if(ro) {
		if(ui->buttonBox->button(QDialogButtonBox::Save))
			ui->buttonBox->removeButton(ui->buttonBox->button(QDialogButtonBox::Save));
	}
	else {
		if(!ui->buttonBox->button(QDialogButtonBox::Save))
			ui->buttonBox->addButton(QDialogButtonBox::Save);
	}
}


