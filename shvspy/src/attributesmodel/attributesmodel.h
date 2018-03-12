#ifndef ATTRIBUTESMODEL_H
#define ATTRIBUTESMODEL_H

//#include <qfopcua/client.h>
//#include <qfopcua/nodeid.h>

#include <QStandardItemModel>
#include <QPointer>

class AttributeNode;

class AttributesModel : public QStandardItemModel
{
	Q_OBJECT
private:
	typedef QStandardItemModel Super;
public:
	AttributesModel(QObject *parent = nullptr);
	~AttributesModel() Q_DECL_OVERRIDE;
public:
	Qt::ItemFlags flags(const QModelIndex &ix) const Q_DECL_OVERRIDE;
	QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;
	bool setData(const QModelIndex &ix, const QVariant &val, int role = Qt::EditRole) Q_DECL_OVERRIDE;
	QVariant headerData ( int section, Qt::Orientation o, int role = Qt::DisplayRole ) const Q_DECL_OVERRIDE;

	//void setNode(qfopcua::Client *client, const qfopcua::NodeId &node_id);
	//qfopcua::DataValue attribute(qfopcua::AttributeId::Enum attr_id) const;
	//qfopcua::NodeId nodeId() const {return m_nodeId;}
private:
	void load();
	void appendNode(AttributeNode *nd, bool load = true);
	//AttributeNode* createNode(qfopcua::AttributeId::Enum attr_id);
private:
	//QPointer<qfopcua::Client> m_client;
	//qfopcua::NodeId m_nodeId;
	int m_userAccessLevel = 0;
};

#endif // ATTRIBUTESMODEL_H
