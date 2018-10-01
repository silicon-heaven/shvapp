#include "lastusedparamswidget.h"
#include "theapp.h"
#include "ui_lastusedparamswidget.h"

LastUsedParamsWidget::LastUsedParamsWidget(const QString &path, const QString &method, QWidget *parent)
	: QWidget(parent)
	, ui(new Ui::LastUsedParamsWidget)
{
	ui->setupUi(this);

	auto &param_map = TheApp::instance()->lastUsedParams();

	ui->thisTable->setRowCount(param_map[path][method].count());
	QVector<const TheApp::CallParam*> this_params;
	for (const TheApp::CallParam &param : param_map[path][method]) {
		this_params << &param;
	}
	std::sort(this_params.begin(), this_params.end(), [](const TheApp::CallParam *par1, const TheApp::CallParam *par2) {
		return par1->time > par2->time;
	});
	int row = 0;
	for (const TheApp::CallParam *param : this_params) {
		QTableWidgetItem *item = new QTableWidgetItem(param->param);
		item->setFlags(item->flags() & ~Qt::ItemFlag::ItemIsEditable);
		ui->thisTable->setItem(row++, 0, item);
	}

	QVector<const TheApp::CallParam*> all_params;
	for (auto path_it = param_map.cbegin(); path_it != param_map.cend(); ++path_it) {
		const auto &path_item = path_it.value();
		for (auto method_it = path_item.cbegin(); method_it != path_item.cend(); ++method_it) {
			const auto &method_item = method_it.value();
			for (const TheApp::CallParam &param_item : method_item) {
				if (path_it.key() != path || method_it.key() != method) {
					all_params << &param_item;
				}
			}
		}
	}
	std::sort(all_params.begin(), all_params.end(), [](const TheApp::CallParam *par1, const TheApp::CallParam *par2) {
		return par1->time > par2->time;
	});
	ui->othersTable->setRowCount(all_params.count() > 5 ? 5 : all_params.count());
	row = 0;
	for (const TheApp::CallParam *param : all_params) {
		QTableWidgetItem *item = new QTableWidgetItem(param->param);
		item->setFlags(item->flags() & ~Qt::ItemFlag::ItemIsEditable);
		ui->othersTable->setItem(row++, 0, item);
		if (row > 5) {
			break;
		}
	}

	connect(ui->thisTable, &QTableWidget::cellDoubleClicked, [this](int row, int col) {
		Q_UNUSED(col);
		Q_EMIT paramSelected(ui->thisTable->item(row, 0)->text());
	});
	connect(ui->othersTable, &QTableWidget::cellDoubleClicked, [this](int row, int col) {
		Q_UNUSED(col);
		Q_EMIT paramSelected(ui->othersTable->item(row, 0)->text());
	});
	connect(ui->closeButton, &QPushButton::clicked, this, &LastUsedParamsWidget::close);
}

LastUsedParamsWidget::~LastUsedParamsWidget()
{
	delete ui;
}
