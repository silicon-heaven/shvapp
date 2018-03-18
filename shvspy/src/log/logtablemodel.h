#pragma once

#include <shv/coreqt/utils.h>

#include <QAbstractTableModel>

class LogTableModelRow : public QVector<QVariant>
{
public:
	//explicit LogTableModelRow() {}
	//explicit Row(qf::core::Log::Level severity, const QString& domain, const QString& file, int line, const QString& msg, const QDateTime& time_stamp, const QString& function = QString(), const QVariant &user_data = QVariant());
};

class LogTableModel : public QAbstractTableModel
{
	Q_OBJECT
private:
	typedef QAbstractTableModel Super;
public:
	LogTableModel(QObject *parent = 0);

	enum class Direction {AppendToTop, AppendToBottom};

	SHV_PROPERTY_IMPL2(Direction, d, D, irection, Direction::AppendToBottom)
	SHV_PROPERTY_IMPL2(int, m, M, aximumRowCount, 1000)

	//QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
	int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
	//int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;

	void clear();
	LogTableModelRow rowAt(int row) const;
	void addLogRow(const LogTableModelRow &row);
	//void addLog(qf::core::Log::Level severity, const QString& category, const QString &file, int line, const QString& msg, const QDateTime& time_stamp, const QString &function = QString(), const QVariant &user_data = QVariant());
	Q_SIGNAL void logRowInserted(int row_no);
protected:
	QList<LogTableModelRow> m_rows;
};


