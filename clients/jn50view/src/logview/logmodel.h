#ifndef LOGMODEL_H
#define LOGMODEL_H

#include <shv/chainpack/rpcvalue.h>

#include <QAbstractTableModel>

class LogModel : public QAbstractTableModel
{
	Q_OBJECT

	using Super = QAbstractTableModel;
public:
	enum {ColDateTime = 0, ColPath, ColValue,  ColCnt};
public:
	LogModel(QObject *parent = nullptr);

	void setLog(const shv::chainpack::RpcValue &log);

	int rowCount(const QModelIndex & = QModelIndex()) const override;
	int columnCount(const QModelIndex & = QModelIndex()) const override {return ColCnt;}
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	QVariant data(const QModelIndex &index, int role) const override;
protected:
	shv::chainpack::RpcValue m_log;
};

#endif // LOGMODEL_H
