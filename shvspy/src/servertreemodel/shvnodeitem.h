#pragma once

#include <QStandardItem>

class ServerNode;

class ShvNodeItem : public QObject, public QStandardItem
{
	Q_OBJECT
private:
	typedef QStandardItem Super;
public:
	ShvNodeItem(const std::string &ndid);
	~ShvNodeItem() Q_DECL_OVERRIDE;

	const std::string& nodeId() const {return m_nodeId;}
	ServerNode* serverNode() const;

	QVariant data(int role = Qt::UserRole + 1) const Q_DECL_OVERRIDE;
	void loadChildren(bool force);
protected:
	//QVariant attribute(qfopcua::AttributeId::Enum attr_id) const;
protected:
	std::string m_nodeId;
	//mutable QMap<qfopcua::AttributeId::Enum, QVariant> m_attribudes;
	bool m_childrenLoaded = false;
};

