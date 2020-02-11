#pragma once

#include <shv/chainpack/rpcvalue.h>
#include <QAbstractTableModel>

class PathsModel : public QAbstractTableModel
{
	Q_OBJECT
private:
	typedef QAbstractTableModel Super;
public:
	enum Columns {ColPath = 0, ColGrant, ColWeight, /*ColForwardGrant,*/ ColCount};

public:
	PathsModel(QObject *parent = nullptr);
	~PathsModel() Q_DECL_OVERRIDE;
public:
	void setPaths(const shv::chainpack::RpcValue::Map &paths);
	shv::chainpack::RpcValue::Map paths();

	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override;
	Qt::ItemFlags flags(const QModelIndex &ix) const Q_DECL_OVERRIDE;
	QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;
	bool setData(const QModelIndex &ix, const QVariant &val, int role = Qt::EditRole) Q_DECL_OVERRIDE;
	QVariant headerData (int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;
	void addPath();
	void deletePath(int index);

private:
	QVector <shv::chainpack::RpcValue::Map> m_paths;
};
