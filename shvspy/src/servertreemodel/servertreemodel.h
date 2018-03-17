#pragma once

#include "shvnodeitem.h"

#include <QAbstractItemModel>

class ShvBrokerNodeItem;
class QSettings;

class ServerTreeModel : public QAbstractItemModel
{
	Q_OBJECT
private:
	typedef QAbstractItemModel Super;
	friend class ShvNodeItem;
public:
	ServerTreeModel(QObject *parent = nullptr);
	~ServerTreeModel() Q_DECL_OVERRIDE;
public:
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
	QModelIndex parent(const QModelIndex &child) const override;
	bool hasChildren(const QModelIndex &parent) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
	int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;
	QVariant headerData ( int section, Qt::Orientation o, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;

	ShvNodeItem* itemFromIndex(const QModelIndex &ix) const;
	QModelIndex indexFromItem(ShvNodeItem *nd) const;
	ShvNodeRootItem* invisibleRootItem() const {return m_invisibleRoot;}

	void loadSettings(const QSettings &settings);
	void saveSettings(QSettings &settings);
public:
	ShvBrokerNodeItem* createConnection(const QVariantMap &params);
	unsigned nextId() {return ++m_maxId;}
private:
	ShvNodeRootItem *m_invisibleRoot;
	QMap<unsigned, ShvNodeItem*> m_nodes;
	unsigned m_maxId = 0;
};

