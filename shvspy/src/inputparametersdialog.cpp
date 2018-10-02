#include "inputparametersdialog.h"
#include "lastusedparamswidget.h"
#include "ui_inputparametersdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QLineEdit>

#include <shv/chainpack/rpcvalue.h>

QVector<shv::chainpack::RpcValue::Type> InputParametersDialog::m_supportedTypes {
	shv::chainpack::RpcValue::Type::String,
	shv::chainpack::RpcValue::Type::Int,
	shv::chainpack::RpcValue::Type::Double,
	shv::chainpack::RpcValue::Type::Bool,
	shv::chainpack::RpcValue::Type::DateTime,
};

InputParametersDialog::InputParametersDialog(const QString &path, const QString &method, const shv::chainpack::RpcValue &params, QWidget *parent)
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

shv::chainpack::RpcValue InputParametersDialog::value() const
{
	if (ui->tabWidget->currentIndex() == 0) {
		return paramValue();
	}
	else {
		std::string cpon = ui->rawCponEdit->toPlainText().toStdString();
		if (cpon.size()) {
			std::string err;
			shv::chainpack::RpcValue val = shv::chainpack::RpcValue::fromCpon(ui->rawCponEdit->toPlainText().toStdString(), &err);
			if (err.size() == 0) {
				return val;
			}
		}
		return shv::chainpack::RpcValue();
	}
}

void InputParametersDialog::newParameter()
{
	newParameter(shv::chainpack::RpcValue());
}

void InputParametersDialog::newParameter(const shv::chainpack::RpcValue &param)
{
	int row = ui->parameterTable->rowCount();
	ui->parameterTable->setRowCount(row + 1);

	QComboBox *combo = new QComboBox(this);
	ui->parameterTable->setCellWidget(row, 0, combo);
	for (shv::chainpack::RpcValue::Type t : m_supportedTypes) {
		combo->addItem(shv::chainpack::RpcValue::typeToName(t), (int)t);
	}
	m_cellValueGetters << ValueGetter();
	m_cellValueSetters << ValueSetter();
	ui->parameterTable->setVerticalHeaderItem(row, new QTableWidgetItem("  "));

	if (param.isValid()) {
		shv::chainpack::RpcValue::Type t = param.type();
		int index = m_supportedTypes.indexOf(t);
		combo->setCurrentIndex(index);
		switchByType(t, row);
		m_cellValueSetters[index](param);
	}
	else {
		int index = m_supportedTypes.indexOf(shv::chainpack::RpcValue::Type::String);
		combo->setCurrentIndex(index);
		switchToString(row);
	}
	connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, combo, row](int index) {
		shv::chainpack::RpcValue::Type t = (shv::chainpack::RpcValue::Type)combo->itemData(index).toInt();
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

bool InputParametersDialog::tryParseParams(const shv::chainpack::RpcValue &params)
{
	if (!params.isList()) {
		return false;
	}
	shv::chainpack::RpcValue::List param_list = params.toList();

	for (const shv::chainpack::RpcValue &param : param_list) {
		if (!m_supportedTypes.contains(param.type())) {
			return false;
		}
	}
	for (const shv::chainpack::RpcValue &param : param_list) {
		newParameter(param);
	}
	return true;
}

void InputParametersDialog::switchToBool(int row)
{
	QCheckBox *checkbox = new QCheckBox(this);
	ui->parameterTable->setCellWidget(row, 1, checkbox);
	m_cellValueGetters[row] = [checkbox]() {
		return shv::chainpack::RpcValue(checkbox->isChecked() ? true : false);
	};
	m_cellValueSetters[row] = [checkbox](const shv::chainpack::RpcValue &param) {
		checkbox->setChecked(param.toBool());
	};
}

void InputParametersDialog::switchToInt(int row)
{
	QLineEdit *line_edit = new QLineEdit(this);
	ui->parameterTable->setCellWidget(row, 1, line_edit);
	line_edit->setValidator(new QIntValidator);
	m_cellValueGetters[row] = [line_edit]() {
		return shv::chainpack::RpcValue(line_edit->text().toInt());
	};
	m_cellValueSetters[row] = [line_edit](const shv::chainpack::RpcValue &param) {
		line_edit->setText(QString::number(param.toInt()));
	};
}

void InputParametersDialog::switchToString(int row)
{
	QLineEdit *line_edit = new QLineEdit(this);
	ui->parameterTable->setCellWidget(row, 1, line_edit);
	m_cellValueGetters[row] = [line_edit]() {
		return  shv::chainpack::RpcValue(line_edit->text().toStdString());
	};
	m_cellValueSetters[row] = [line_edit](const shv::chainpack::RpcValue &param) {
		line_edit->setText(QString::fromStdString(param.toString()));
	};
}

void InputParametersDialog::switchToDouble(int row)
{
	QLineEdit *line_edit = new QLineEdit(this);
	ui->parameterTable->setCellWidget(row, 1, line_edit);
	line_edit->setValidator(new QDoubleValidator);
	m_cellValueGetters[row] = [line_edit]() {
		return shv::chainpack::RpcValue(line_edit->text().toDouble());
	};
	m_cellValueSetters[row] = [line_edit](const shv::chainpack::RpcValue &param) {
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
		return shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(edit->dateTime().toMSecsSinceEpoch());
	};
	connect(today, &QPushButton::clicked, [edit]() {
		edit->setDateTime(QDateTime::currentDateTime());
	});
	m_cellValueSetters[row] = [edit](const shv::chainpack::RpcValue &param) {
		edit->setDateTime(QDateTime::fromMSecsSinceEpoch(param.toDateTime().msecsSinceEpoch()));
	};
}

void InputParametersDialog::switchByType(const shv::chainpack::RpcValue::Type &type, int row)
{
	if (type == shv::chainpack::RpcValue::Type::Int) {
		switchToInt(row);
	}
	else if (type == shv::chainpack::RpcValue::Type::Bool) {
		switchToBool(row);
	}
	else if (type == shv::chainpack::RpcValue::Type::String) {
		switchToString(row);
	}
	else if (type == shv::chainpack::RpcValue::Type::Double) {
		switchToDouble(row);
	}
	else if (type == shv::chainpack::RpcValue::Type::DateTime) {
		switchToDateTime(row);
	}
}

void InputParametersDialog::clearParams()
{
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
			shv::chainpack::RpcValue value = paramValue();
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
			shv::chainpack::RpcValue val = shv::chainpack::RpcValue::fromCpon(cpon, &err);
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
		shv::chainpack::RpcValue val = shv::chainpack::RpcValue::fromCpon(ui->rawCponEdit->toPlainText().toStdString(), &err);
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
	shv::chainpack::RpcValue params = shv::chainpack::RpcValue::fromCpon(s.toStdString());
	if (params.isValid()) {
		ui->tabWidget->setCurrentIndex(0);
		if (!tryParseParams(params)) {
			ui->tabWidget->setCurrentIndex(1);
			ui->rawCponEdit->setPlainText(QString::fromStdString(params.toPrettyString("    ")));
		}
	}
}

shv::chainpack::RpcValue InputParametersDialog::paramValue() const
{
	if (ui->parameterTable->rowCount() == 0) {
		return shv::chainpack::RpcValue();
	}
	shv::chainpack::RpcValue::List list;
	for (int i = 0; i < ui->parameterTable->rowCount(); ++i) {
		list.push_back(m_cellValueGetters[i]());
	}
	return list;
}
