#pragma once

#include <QStandardItemModel>

class ServerNode;
class QSettings;

class ServerTreeModel : public QStandardItemModel
{
	Q_OBJECT
private:
	typedef QStandardItemModel Super;
public:
	ServerTreeModel(QObject *parent = nullptr);
	~ServerTreeModel() Q_DECL_OVERRIDE;
public:
	int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
	int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
	QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;
	QVariant headerData ( int section, Qt::Orientation o, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;

	void loadSettings(const QSettings &settings);
	void saveSettings(QSettings &settings);
public:
	ServerNode* createConnection(const QVariantMap &params);
	ServerNode* connectionForOid(int oid);
private:
};

