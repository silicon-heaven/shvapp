#include "texteditdialog.h"
#include "ui_texteditdialog.h"

#include <shv/chainpack/rpcvalue.h>

#include <QSettings>
#include <QPushButton>
#include <QTimer>

namespace cp = shv::chainpack;

//=========================================================
// TextEditDialog
//=========================================================
TextEditDialog::TextEditDialog(QWidget *parent)
	: Super(parent)
	, ui(new Ui::TextEditDialog)
{
	ui->setupUi(this);
	ui->lblError->hide();
	ui->btFormatCpon->hide();
	ui->btCompactCpon->hide();
	setReadOnly(false);

	QSettings settings;
	restoreGeometry(settings.value(QStringLiteral("ui/ResultView/geometry")).toByteArray());

	ui->plainTextEdit->setFocus();
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

//=========================================================
// CponEditDialog
//=========================================================
CponEditDialog::CponEditDialog(QWidget *parent)
	: Super(parent)
{
	ui->btFormatCpon->show();
	ui->btCompactCpon->show();

	connect(ui->plainTextEdit, &QPlainTextEdit::textChanged, this, &CponEditDialog::validateContentDeferred);
	connect(ui->btCompactCpon, &QPushButton::clicked, this, &CponEditDialog::onBtCompactCponClicked);
	connect(ui->btFormatCpon, &QPushButton::clicked, this, &CponEditDialog::onBtFormatCponClicked);
}

void CponEditDialog::setValidateContent(bool b)
{
	m_isValidateContent = b;
}

void CponEditDialog::validateContentDeferred()
{
	if(!m_isValidateContent)
		return;
	if(!m_validateTimer) {
		m_validateTimer = new QTimer(this);
		m_validateTimer->setSingleShot(true);
		m_validateTimer->setInterval(1000);
		connect(m_validateTimer, &QTimer::timeout, this, &CponEditDialog::validateContent);
	}
	m_validateTimer->start();
}

cp::RpcValue CponEditDialog::validateContent()
{
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

void CponEditDialog::onBtCompactCponClicked()
{
	shv::chainpack::RpcValue rv = validateContent();
	if(rv.isValid()) {
		std::string cpon = rv.toCpon();
		setText(QString::fromStdString(cpon));
	}
}

void CponEditDialog::onBtFormatCponClicked()
{
	shv::chainpack::RpcValue rv = validateContent();
	if(rv.isValid()) {
		std::string cpon = rv.toCpon("  ");
		setText(QString::fromStdString(cpon));
	}
}

