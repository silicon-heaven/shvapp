#include "logwidget.h"
#include "ui_logwidget.h"

#include "logtablemodel.h"


#include <shv/coreqt/log.h>
#include <shv/core/exception.h>
//#include <qf/core/logdevice.h>

#include <QSortFilterProxyModel>
#include <QDateTime>
#include <QClipboard>
#include <QKeyEvent>
#include <QMenu>
#include <QInputDialog>
#include <QScrollBar>

LogWidgetTableView::LogWidgetTableView(QWidget *parent)
	: Super(parent)
{
	QAction *a;
	{
		a = new QAction(tr("Copy"), this);
		a->setIcon(QIcon(QStringLiteral(":/shvspy/images/copy")));
		a->setShortcut(QKeySequence(tr("Ctrl+C", "Copy selection")));
		a->setShortcutContext(Qt::WidgetShortcut);
		connect(a, &QAction::triggered, this, &LogWidgetTableView::copy);
		addAction(a);
	}
	setContextMenuPolicy(Qt::ActionsContextMenu);
}

void LogWidgetTableView::copy()
{
	shvLogFuncFrame();
	auto *m = model();
	if(!m)
		return;
	int n = 0;
	QString rows;
	QItemSelection sel = selectionModel()->selection();
	foreach(const QItemSelectionRange &sel1, sel) {
		if(sel1.isValid()) {
			for(int row=sel1.top(); row<=sel1.bottom(); row++) {
				QString cells;
				for(int col=sel1.left(); col<=sel1.right(); col++) {
					QModelIndex ix = m->index(row, col);
					QString s;
					s = ix.data(Qt::DisplayRole).toString();
					static constexpr bool replace_escapes = true;
					if(replace_escapes) {
						s.replace('\r', QStringLiteral("\\r"));
						s.replace('\n', QStringLiteral("\\n"));
						s.replace('\t', QStringLiteral("\\t"));
					}
					if(col > sel1.left())
						cells += '\t';
					cells += s;
				}
				if(n++ > 0)
					rows += '\n';
				rows += cells;
			}
		}
	}
	if(!rows.isEmpty()) {
		shvDebug() << "\tSetting clipboard:" << rows;
		QClipboard *clipboard = QApplication::clipboard();
		clipboard->setText(rows);
	}
}

void LogWidgetTableView::keyPressEvent(QKeyEvent *e)
{
	shvLogFuncFrame() << "key:" << e->key() << "modifiers:" << e->modifiers();
	if(e->modifiers() == Qt::ControlModifier) {
		if(e->key() == Qt::Key_C) {
			copy();
			e->accept();
			return;
		}
	}
	Super::keyPressEvent(e);
}

class LogFilterProxyModel : public QSortFilterProxyModel
{
	typedef QSortFilterProxyModel Super;
public:
	LogFilterProxyModel(QObject* parent)
		: Super(parent) {

	}
	/*
	void setThreshold(int level) {
		m_treshold = level;
		invalidateFilter();
	}
	*/
	void setFilterString(const QString &filter_string) {
		m_filterString = filter_string;
		invalidateFilter();
	}
	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const Q_DECL_OVERRIDE;
private:
	//int m_treshold = static_cast<int>(qf::core::Log::Level::Info);
	QString m_filterString;
};

bool LogFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	auto *log_model = qobject_cast<LogTableModel*>(sourceModel());
	//QModelIndex index = log_model->index(source_row, qfm::LogTableModel::Cols::Severity, source_parent);
	//bool ok = (log_model->data(index, Qt::EditRole).toInt() <= m_treshold);
	//shvDebug() << log_model->data(index, Qt::EditRole).toInt() << m_treshold << ok;
	bool ok = true;
	if(!m_filterString.isEmpty()) {
		ok = false;
		QModelIndex index = log_model->index(source_row, 0, source_parent);
		for (int i = 0; i < log_model->columnCount(); ++i) {
			index = index.sibling(index.row(), i);
			QString s = log_model->data(index, Qt::DisplayRole).toString();
			if(s.contains(m_filterString, Qt::CaseInsensitive)) {
				ok = true;
				break;
			}
		}
	}
	return ok;
}

LogWidget::LogWidget(QWidget *parent)
	: Super(parent)
	, ui(new Ui::LogWidget)
{
	ui->setupUi(this);
	//setPersistentSettingsId();
	{
		QAction *a = new QAction(tr("Maximal log length"), this);
		connect(a, &QAction::triggered, [this]() {
			LogTableModel *m = this->logTableModel();
			int max_rows = m->maximumRowCount();
			bool ok;
			max_rows = QInputDialog::getInt(this, tr("Get number"), tr("Maximal log row count:"), max_rows, 0, std::numeric_limits<int>::max(), 100, &ok);
			if (ok)
				m->setMaximumRowCount(max_rows);
		});
		tableMenuButton()->addAction(a);
	}
	{
		QAction *a = new QAction(this);
		a->setSeparator(true);
		tableMenuButton()->addAction(a);
	}

	//ui->tableView->horizontalHeader()->setSectionHidden(0, true);
	ui->tableView->horizontalHeader()->setSectionsMovable(true);
	ui->tableView->verticalHeader()->setDefaultSectionSize((int)(fontMetrics().lineSpacing() * 1.3));
	m_filterModel = new LogFilterProxyModel(this);
	m_filterModel->setDynamicSortFilter(false);
	ui->tableView->setModel(m_filterModel);

	connect(ui->edFilter, &QLineEdit::textChanged, this, &LogWidget::filterStringChanged);
}

LogWidget::~LogWidget()
{
	delete ui;
}

void LogWidget::clear()
{
	if(m_logTableModel)
		m_logTableModel->clear();
}

void LogWidget::setLogTableModel(LogTableModel *m)
{
	if(m_logTableModel != m) {
		m_logTableModel = m;
		m_filterModel->setSourceModel(m_logTableModel);
		if(m_logTableModel) {
			connect(m_logTableModel, &LogTableModel::logRowInserted, this, &LogWidget::scrollToLastRow, Qt::UniqueConnection);
			QScrollBar *sb = ui->tableView->verticalScrollBar();
			if(sb)
				connect(sb, &QScrollBar::valueChanged, this, &LogWidget::onVerticalScrollBarValueChanged, Qt::UniqueConnection);
		}
	}
}

void LogWidget::addLogRow(const LogTableModelRow &row)
{
	logTableModel()->addLogRow(row);
}

LogTableModel *LogWidget::logTableModel()
{
	if(!m_logTableModel)
		SHV_EXCEPTION("Table model is NULL!");
	return m_logTableModel;
}
/*
void LogWidget::setSeverityTreshold(core::Log::Level lvl)
{
	int ci = ui->severityTreshold->findData(static_cast<int>(lvl));
	ui->severityTreshold->setCurrentIndex(ci);
}

void LogWidget::onSeverityTresholdChanged(int index)
{
	Q_UNUSED(index);
	m_filterModel->setThreshold(ui->severityTreshold->currentData().toInt());
	emit severityTresholdChanged(static_cast<qf::core::Log::Level>(ui->severityTreshold->currentData().toInt()));
}
*/
void LogWidget::filterStringChanged(const QString &filter_string)
{
	m_filterModel->setFilterString(filter_string);
}

void LogWidget::on_btClearLog_clicked()
{
	clear();
}

void LogWidget::on_btResizeColumns_clicked()
{
	ui->tableView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
}

QAbstractButton *LogWidget::tableMenuButton()
{
	return ui->btTableMenu;
}

QTableView *LogWidget::tableView() const
{
	return ui->tableView;
}

void LogWidget::onVerticalScrollBarValueChanged()
{
	QScrollBar *sb = ui->tableView->verticalScrollBar();
	if(sb) {
		if(logTableModel()->direction() == LogTableModel::Direction::AppendToBottom) {
			m_isAutoScroll = (sb->value() == sb->maximum());
			//fprintf(stderr, "BOTTOM scrollbar min: %d max: %d value: %d -> %d\n", sb->minimum(), sb->maximum(), sb->value(), m_scrollToLastEntryAfterInsert);
		}
		else {
			m_isAutoScroll = (sb->value() == sb->minimum());
			//fprintf(stderr, "TOP scrollbar min: %d max: %d value: %d -> %d\n", sb->minimum(), sb->maximum(), sb->value(), m_scrollToLastEntryAfterInsert);
		}
	}
}

void LogWidget::scrollToLastRow()
{
	if(isVisible()) {
		if(!m_columnsResized) {
			m_columnsResized = true;
			ui->tableView->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
		}
		if(m_isAutoScroll) {
			if(logTableModel()->direction() == LogTableModel::Direction::AppendToBottom) {
				ui->tableView->scrollToBottom();
			}
			else {
				ui->tableView->scrollToTop();
			}
		}
	}
}
