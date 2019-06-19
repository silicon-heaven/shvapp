#include "logmodel.h"

LogModel::LogModel(QObject *parent)
	: Super(parent)
{

}

void LogModel::setLog(const shv::chainpack::RpcValue &log)
{
	beginResetModel();
	m_log = log;
	endResetModel();
}

int LogModel::rowCount(const QModelIndex &) const
{
	const shv::chainpack::RpcValue::List &lst = m_log.toList();
	return (int)lst.size();
}

QVariant LogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(orientation == Qt::Horizontal) {
		if(role == Qt::DisplayRole) {
			switch (section) {
			case ColDateTime: return tr("DateTime");
			case ColPath: return tr("Path");
			case ColValue: return tr("Value");
			}
		}
	}
	return Super::headerData(section, orientation, role);
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
	if(index.isValid() && index.row() < rowCount()) {
		if(role == Qt::DisplayRole) {
			const shv::chainpack::RpcValue::List &lst = m_log.toList();
			shv::chainpack::RpcValue row = lst.value((unsigned)index.row());
			shv::chainpack::RpcValue val = row.toList().value((unsigned)index.column());
			return QString::fromStdString(val.toCpon());
		}
	}
	return QVariant();
}
