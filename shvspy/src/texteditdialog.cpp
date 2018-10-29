#include "texteditdialog.h"
#include "ui_texteditdialog.h"

#include <shv/chainpack/rpcvalue.h>

#include <QSettings>
#include <QPushButton>
#include <QTimer>

namespace cp = shv::chainpack;

TextEditDialog::TextEditDialog(QWidget *parent)
	: Super(parent)
	, ui(new Ui::TextEditDialog)
{
	ui->setupUi(this);
	ui->lblError->hide();
	ui->frmCponTools->hide();
	setReadOnly(false);

	connect(ui->plainTextEdit, &QPlainTextEdit::textChanged, this, &TextEditDialog::validateContentDeferred);

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
	ui->btSave->setVisible(!ro);
}

void TextEditDialog::setValidateCpon(bool b)
{
	m_isValidateContent = b;
	ui->frmCponTools->setVisible(b);
}

void TextEditDialog::validateContentDeferred()
{
	if(!m_isValidateContent)
		return;
	if(!m_validateTimer) {
		m_validateTimer = new QTimer(this);
		m_validateTimer->setSingleShot(true);
		m_validateTimer->setInterval(1000);
		connect(m_validateTimer, &QTimer::timeout, this, &TextEditDialog::validateContent);
	}
	m_validateTimer->start();
}

cp::RpcValue TextEditDialog::validateContent()
{
	if(!m_isValidateContent)
		return cp::RpcValue();
	QString txt = text().trimmed();
	std::string err;
	cp::RpcValue rv = cp::RpcValue::fromCpon(txt.toStdString(), &err);
	if(txt.isEmpty() || err.empty()) {
		ui->lblError->setVisible(false);
		return rv;
	}
	else {
		ui->lblError->setText(tr("Malformed Cpon: ") + QString::fromStdString(err));
		ui->lblError->setVisible(true);
		return cp::RpcValue();
	}
}

void TextEditDialog::on_btCompactCpon_clicked()
{
	shv::chainpack::RpcValue rv = validateContent();
	if(rv.isValid()) {
		std::string cpon = rv.toCpon();
		setText(QString::fromStdString(cpon));
	}
}

void TextEditDialog::on_btFormatCpon_clicked()
{
	shv::chainpack::RpcValue rv = validateContent();
	if(rv.isValid()) {
		std::string cpon = rv.toCpon("  ");
		setText(QString::fromStdString(cpon));
	}
}

