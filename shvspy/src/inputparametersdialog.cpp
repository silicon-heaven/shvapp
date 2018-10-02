#include "inputparametersdialog.h"
#include "lastusedparamswidget.h"
#include "ui_inputparametersdialog.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/coreqt/log.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QLineEdit>

namespace cp = shv::chainpack;

QVector<cp::RpcValue::Type> InputParametersDialog::m_supportedTypes {
	cp::RpcValue::Type::String,
	cp::RpcValue::Type::Int,
	cp::RpcValue::Type::UInt,
	cp::RpcValue::Type::Double,
	cp::RpcValue::Type::Bool,
	cp::RpcValue::Type::DateTime,
};

InputParametersDialog::InputParametersDialog(const QString &path, const QString &method, const cp::RpcValue &params, QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::InputParametersDialog)
	, m_syntaxCheckTimer(this)
	, m_path(path)
	, m_method(method)
	, m_usedParamsWidget(nullptr)
{
	ui->setupUi(this);

	ui->removeButton->setEnabled(false);
	ui->parsingLabel->hide();

	connect(ui->tabWidget, &QTabWidget::currentChanged, this, &InputParametersDialog::onCurrentTabChanged);
	connect(ui->addButton, &QPushButton::clicked, this, QOverload<>::of(&InputParametersDialog::newParameter));
	connect(ui->removeButton, &QPushButton::clicked, this, &InputParametersDialog::removeParameter);
	connect(ui->lastUsedButton, &QPushButton::clicked, this, &InputParametersDialog::loadLastUsed);
	connect(ui->clearButton, &QPushButton::clicked, this, &InputParametersDialog::clear);
	connect(ui->parameterTable, &QTableWidget::currentCellChanged, this, &InputParametersDialog::onCurrentCellChanged);
	connect(ui->rawCponEdit, &QPlainTextEdit::textChanged, &m_syntaxCheckTimer, QOverload<>::of(&QTimer::start));
	if (params.isValid()) {
		if (!tryParseParams(params)) {
			ui->tabWidget->setCurrentIndex(1);
			ui->rawCponEdit->setPlainText(QString::fromStdString(params.toPrettyString("    ")));
		}
	}
	m_syntaxCheckTimer.setInterval(500);
	m_syntaxCheckTimer.setSingleShot(true);
	connect(&m_syntaxCheckTimer, &QTimer::timeout, this, &InputParametersDialog::checkSyntax);
}

InputParametersDialog::~InputParametersDialog()
{
	delete ui;
}

cp::RpcValue InputParametersDialog::value() const
{
	if (ui->tabWidget->currentIndex() == 0) {
		return paramValue();
	}
	else {
		std::string cpon = ui->rawCponEdit->toPlainText().toStdString();
		if (cpon.size()) {
			std::string err;
			cp::RpcValue val = cp::RpcValue::fromCpon(ui->rawCponEdit->toPlainText().toStdString(), &err);
			if (err.size() == 0) {
				return val;
			}
		}
		return cp::RpcValue();
	}
}

void InputParametersDialog::newParameter()
{
	newParameter(cp::RpcValue());
}

void InputParametersDialog::newParameter(const cp::RpcValue &param)
{
	int row = ui->parameterTable->rowCount();
	ui->parameterTable->setRowCount(row + 1);

	QComboBox *combo = new QComboBox(this);
	ui->parameterTable->setCellWidget(row, 0, combo);
	for (cp::RpcValue::Type t : m_supportedTypes) {
		combo->addItem(cp::RpcValue::typeToName(t), (int)t);
	}
	m_cellValueGetters << ValueGetter();
	m_cellValueSetters << ValueSetter();
	ui->parameterTable->setVerticalHeaderItem(row, new QTableWidgetItem("  "));

	if (param.isValid()) {
		cp::RpcValue::Type t = param.type();
		int index = m_supportedTypes.indexOf(t);
		combo->setCurrentIndex(index);
		switchByType(t, row);
		m_cellValueSetters[row](param);
	}
	else {
		int index = m_supportedTypes.indexOf(cp::RpcValue::Type::String);
		combo->setCurrentIndex(index);
		switchToString(row);
	}
	connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, combo, row](int index) {
		cp::RpcValue::Type t = (cp::RpcValue::Type)combo->itemData(index).toInt();
		switchByType(t, row);
	});
}

void InputParametersDialog::onCurrentCellChanged(int row, int col)
{
	Q_UNUSED(col);
	ui->removeButton->setEnabled(row != -1);
}

void InputParametersDialog::removeParameter()
{
	ui->parameterTable->removeRow(ui->parameterTable->currentRow());
	m_cellValueGetters.removeAt(ui->parameterTable->currentRow());
}

bool InputParametersDialog::tryParseParams(const cp::RpcValue &params)
{
	if (!params.isList()) {
		return false;
	}
	cp::RpcValue::List param_list = params.toList();

	for (const cp::RpcValue &param : param_list) {
		if (!m_supportedTypes.contains(param.type())) {
			return false;
		}
	}
	for (const cp::RpcValue &param : param_list) {
		newParameter(param);
	}
	return true;
}

void InputParametersDialog::switchToBool(int row)
{
	QCheckBox *checkbox = new QCheckBox(this);
	ui->parameterTable->setCellWidget(row, 1, checkbox);
	m_cellValueGetters[row] = [checkbox]() {
		return cp::RpcValue(checkbox->isChecked() ? true : false);
	};
	m_cellValueSetters[row] = [checkbox](const cp::RpcValue &param) {
		checkbox->setChecked(param.toBool());
	};
}

void InputParametersDialog::switchToInt(int row)
{
	QLineEdit *line_edit = new QLineEdit(this);
	ui->parameterTable->setCellWidget(row, 1, line_edit);
	line_edit->setValidator(new QIntValidator(line_edit));
	m_cellValueGetters[row] = [line_edit]() {
		return cp::RpcValue(line_edit->text().toInt());
	};
	m_cellValueSetters[row] = [line_edit](const cp::RpcValue &param) {
		line_edit->setText(QString::number(param.toInt()));
	};
}

void InputParametersDialog::switchToUInt(int row)
{
	QLineEdit *line_edit = new QLineEdit(this);
	ui->parameterTable->setCellWidget(row, 1, line_edit);
	line_edit->setValidator(new QIntValidator(0, std::numeric_limits<int>::max(), line_edit));
	m_cellValueGetters[row] = [line_edit]() {
		return cp::RpcValue(line_edit->text().toUInt());
	};
	m_cellValueSetters[row] = [line_edit](const cp::RpcValue &param) {
		line_edit->setText(QString::number(param.toUInt()));
	};
}

void InputParametersDialog::switchToString(int row)
{
	QLineEdit *line_edit = new QLineEdit(this);
	ui->parameterTable->setCellWidget(row, 1, line_edit);
	m_cellValueGetters[row] = [line_edit]() {
		return  cp::RpcValue(line_edit->text().toStdString());
	};
	m_cellValueSetters[row] = [line_edit](const cp::RpcValue &param) {
		line_edit->setText(QString::fromStdString(param.toString()));
	};
}

void InputParametersDialog::switchToDouble(int row)
{
	QLineEdit *line_edit = new QLineEdit(this);
	ui->parameterTable->setCellWidget(row, 1, line_edit);
	QDoubleValidator *v = new QDoubleValidator(line_edit);
	v->setLocale(QLocale::C);
	line_edit->setValidator(v);
	m_cellValueGetters[row] = [line_edit]() {
		return cp::RpcValue(line_edit->text().toDouble());
	};
	m_cellValueSetters[row] = [line_edit](const cp::RpcValue &param) {
		line_edit->setText(QString::number(param.toDouble()));
	};
}

void InputParametersDialog::switchToDateTime(int row)
{
	QWidget *datetime_widget = new QWidget(this);
	QDateTimeEdit *edit = new QDateTimeEdit(datetime_widget);
	QPushButton *today = new QPushButton("...", datetime_widget);
	today->setFixedWidth(20);
	QHBoxLayout *layout = new QHBoxLayout;
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(0);
	layout->addWidget(edit);
	layout->addWidget(today);
	datetime_widget->setLayout(layout);
	edit->setDisplayFormat("dd.MM.yyyy HH:mm:ss");
	edit->setCalendarPopup(true);
	ui->parameterTable->setCellWidget(row, 1, datetime_widget);
	m_cellValueGetters[row] = [edit]() {
		return cp::RpcValue::DateTime::fromMSecsSinceEpoch(edit->dateTime().toMSecsSinceEpoch());
	};
	connect(today, &QPushButton::clicked, [edit]() {
		edit->setDateTime(QDateTime::currentDateTime());
	});
	m_cellValueSetters[row] = [edit](const cp::RpcValue &param) {
		edit->setDateTime(QDateTime::fromMSecsSinceEpoch(param.toDateTime().msecsSinceEpoch()));
	};
}

void InputParametersDialog::switchByType(const cp::RpcValue::Type &type, int row)
{
	switch(type) {
	case cp::RpcValue::Type::Int:
		switchToInt(row);
		break;
	case cp::RpcValue::Type::UInt:
		switchToUInt(row);
		break;
	case cp::RpcValue::Type::Bool:
		switchToBool(row);
		break;
	case cp::RpcValue::Type::String:
		switchToString(row);
		break;
	case cp::RpcValue::Type::Double:
		switchToDouble(row);
		break;
	case cp::RpcValue::Type::DateTime:
		switchToDateTime(row);
		break;
	default:
		break;
	}
}

void InputParametersDialog::clearParams()
{
	ui->parameterTable->reset();
	ui->parameterTable->clearContents();
	ui->parameterTable->setRowCount(0);
	ui->removeButton->setEnabled(false);
	ui->addButton->setEnabled(true);
	ui->parsingLabel->hide();
	ui->parameterTable->show();
}

void InputParametersDialog::clear()
{
	clearParams();
	ui->rawCponEdit->clear();
}

void InputParametersDialog::onCurrentTabChanged(int index)
{
	if (index == 1) {
		if (!ui->parameterTable->isHidden()) {
			cp::RpcValue value = paramValue();
			if (value.isValid()) {
				ui->rawCponEdit->setPlainText(QString::fromStdString(value.toPrettyString("    ")));
			}
			else {
				ui->rawCponEdit->setPlainText(QString());
			}
		}
	}
	else {
		clearParams();
		std::string cpon = ui->rawCponEdit->toPlainText().toStdString();
		if (cpon.size() == 0) {
			return;
		}
		else {
			std::string err;
			cp::RpcValue val = cp::RpcValue::fromCpon(cpon, &err);
			if (err.size() == 0) {
				if (tryParseParams(val)) {
					return;
				}
			}
		}
		ui->addButton->setEnabled(false);
		ui->parameterTable->hide();
		ui->parsingLabel->show();
	}
}

void InputParametersDialog::checkSyntax()
{
	std::string cpon = ui->rawCponEdit->toPlainText().toStdString();
	if (cpon.size()) {
		std::string err;
		cp::RpcValue val = cp::RpcValue::fromCpon(ui->rawCponEdit->toPlainText().toStdString(), &err);
		if (err.size()) {
			QPalette pal = ui->rawCponEdit->palette();
			pal.setColor(QPalette::ColorRole::Text, Qt::red);
			ui->rawCponEdit->setPalette(pal);
			return;
		}
	}
	QPalette pal = ui->rawCponEdit->palette();
	pal.setColor(QPalette::ColorRole::Text, Qt::black);
	ui->rawCponEdit->setPalette(pal);
}

void InputParametersDialog::loadLastUsed()
{
	if (!m_usedParamsWidget) {
		m_usedParamsWidget = new LastUsedParamsWidget(m_path, m_method, this);
		m_usedParamsWidget->setWindowFlags(Qt::Popup);
		connect(m_usedParamsWidget, &LastUsedParamsWidget::paramSelected, this, &InputParametersDialog::loadParams);

	}
	m_usedParamsWidget->move(mapToGlobal(ui->lastUsedButton->geometry().bottomRight()));
	m_usedParamsWidget->show();
}

void InputParametersDialog::loadParams(const QString &s)
{
	clear();
	cp::RpcValue params = cp::RpcValue::fromCpon(s.toStdString());
	if (params.isValid()) {
		ui->tabWidget->setCurrentIndex(0);
		if (!tryParseParams(params)) {
			ui->tabWidget->setCurrentIndex(1);
			ui->rawCponEdit->setPlainText(QString::fromStdString(params.toPrettyString("    ")));
		}
	}
}

cp::RpcValue InputParametersDialog::paramValue() const
{
	if (ui->parameterTable->rowCount() == 0) {
		return cp::RpcValue();
	}
	cp::RpcValue::List list;
	for (int i = 0; i < ui->parameterTable->rowCount(); ++i) {
		list.push_back(m_cellValueGetters[i]());
	}
	return list;
}
