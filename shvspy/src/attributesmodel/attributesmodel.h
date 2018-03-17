#pragma once

#include <QAbstractTableModel>

#include <QPointer>

class ShvNodeItem;
//class ShvMetaMethod;

namespace shv { namespace chainpack { class RpcMessage; }}

class AttributesModel : public QAbstractTableModel
{
	Q_OBJECT
private:
	typedef QAbstractTableModel Super;
public:
	enum Columns {ColMethodName = 0, ColParams, ColResult, ColCnt};
public:
	AttributesModel(QObject *parent = nullptr);
	~AttributesModel() Q_DECL_OVERRIDE;
public:
	int rowCount(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent) const override {Q_UNUSED(parent) return ColCnt;}
	Qt::ItemFlags flags(const QModelIndex &ix) const Q_DECL_OVERRIDE;
	QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;
	//bool setData(const QModelIndex &ix, const QVariant &val, int role = Qt::EditRole) Q_DECL_OVERRIDE;
	QVariant headerData ( int section, Qt::Orientation o, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;

	void load(ShvNodeItem *nd);
private:
	//void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void onMethodsLoaded();
private:
	QPointer<ShvNodeItem> m_shvTreeNodeItem;
};
