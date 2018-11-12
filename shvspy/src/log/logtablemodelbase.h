#pragma once

#include <shv/coreqt/utils.h>

#include <QAbstractTableModel>

class LogTableModelRow : public QVector<QVariant>
{
public:
	explicit LogTableModelRow(int size = 0);
};

class LogTableModelBase : public QAbstractTableModel
{
	Q_OBJECT
private:
	typedef QAbstractTableModel Super;
public:
	LogTableModelBase(QObject *parent = nullptr);

	enum class Direction {AppendToTop, AppendToBottom};

	SHV_PROPERTY_IMPL2(Direction, d, D, irection, Direction::AppendToBottom)
	SHV_PROPERTY_IMPL2(int, m, M, aximumRowCount, 1000)

	int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;

	void clear();
	LogTableModelRow rowAt(int row) const;
	void addLogRow(const LogTableModelRow &row);
	Q_SIGNAL void logRowInserted(int row_no);
protected:
	QList<LogTableModelRow> m_rows;
};


