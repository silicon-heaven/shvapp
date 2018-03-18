#include "logtablemodel.h"

//#include "../core/logdevice.h"

#include <QColor>
#include <QDateTime>
/*
LogTableModel::Row::Row(qf::core::Log::Level severity, const QString &domain, const QString &file, int line, const QString &msg, const QDateTime &time_stamp, const QString &function, const QVariant &user_data)
{
	m_data.resize(Cols::Count);
	m_data[Cols::Severity] = QVariant::fromValue(severity);
	m_data[Cols::Category] = domain;
	m_data[Cols::File] = file;
	m_data[Cols::Line] = line;
	m_data[Cols::Function] = function;
	m_data[Cols::Message] = msg;
	m_data[Cols::TimeStamp] = time_stamp;
	m_data[Cols::UserData] = user_data;
}

QVariant LogTableModelRow::value(int col) const
{
	QVariant val = m_data.value(col);
	return val;
}
*/
LogTableModel::LogTableModel(QObject *parent)
	: Super(parent)
{
}
/*
QVariant LogTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		switch (section) {
		case Cols::Category:
			return tr("Category");
		case Cols::File:
			return tr("File");
		case Cols::Line:
			return tr("Line");
		case Cols::Severity:
			return tr("Severity");
		case Cols::TimeStamp:
			return tr("Time stamp");
		case Cols::Message:
			return tr("Message");
		case Cols::Function:
			return tr("Function");
		case Cols::UserData:
			return tr("Data");
		};
		return Super::headerData(section, orientation, role);
	}
	return Super::headerData(section, orientation, role);
}
*/
int LogTableModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return m_rows.count();
}
/*
int LogTableModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return Cols::Count;
}
*/
QVariant LogTableModel::data(const QModelIndex &index, int role) const
{
	if (index.row() < 0 || index.row() >= rowCount()) {
		return QVariant();
	}
	switch (role) {
	case Qt::DisplayRole: {
		QVariant ret = data(index, Qt::EditRole);
		return ret;
	}
	case Qt::EditRole:
		return m_rows[index.row()].value(index.column());
	case Qt::ForegroundRole: {
		break;
	}
	case Qt::BackgroundRole: {
		break;
	}
	};
	return QVariant();
}

void LogTableModel::clear()
{
	beginResetModel();
	m_rows.clear();
	endResetModel();
}

LogTableModelRow LogTableModel::rowAt(int row) const
{
	return m_rows.value(row);
}

void LogTableModel::addLogRow(const LogTableModelRow &row)
{
	//printf("%p %s %s:%d -> %d\n", this, qPrintable(msg), qPrintable(file), line, (int)severity);
	static constexpr int ROWS_OVERLAP = 100;
	if(rowCount() >= maximumRowCount() + ROWS_OVERLAP) {
		if(direction() == Direction::AppendToBottom) {
			beginRemoveRows(QModelIndex(), 0, ROWS_OVERLAP - 1);
			m_rows = m_rows.mid(ROWS_OVERLAP);
			endRemoveRows();
		}
		else {
			beginRemoveRows(QModelIndex(), rowCount() - ROWS_OVERLAP, rowCount() - 1);
			m_rows = m_rows.mid(0, rowCount() - ROWS_OVERLAP);
			endRemoveRows();
		}
	}
	if(direction() == Direction::AppendToBottom) {
		beginInsertRows(QModelIndex(), rowCount(), rowCount());
		m_rows.append(row);
		endInsertRows();
		emit logRowInserted(rowCount() - 1);
	}
	else {
		beginInsertRows(QModelIndex(), 0, 0);
		m_rows.insert(0, row);
		endInsertRows();
		emit logRowInserted(0);
	}
}

