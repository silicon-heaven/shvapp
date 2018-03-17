#pragma once

#include <QObject>
#include <QVector>

class ShvBrokerNodeItem;
class ServerTreeModel;

namespace shv { namespace chainpack { class RpcMessage; } }

class ShvNodeItem : public QObject
{
	Q_OBJECT
private:
	typedef QObject Super;
public:
	ShvNodeItem(ServerTreeModel *m, const std::string &ndid, ShvNodeItem *parent = nullptr);
	~ShvNodeItem() Q_DECL_OVERRIDE;

	ServerTreeModel* treeModel() const;
	const std::string& nodeId() const {return m_nodeId;}
	ShvBrokerNodeItem* serverNode() const;
	ShvNodeItem* parentNode() const;
	ShvNodeItem* childAt(int ix) const;
	int childCount() const {return m_children.count();}
	void insertChild(int ix, ShvNodeItem *n);
	void appendChild(ShvNodeItem *n) {insertChild(m_children.count(), n);}
	ShvNodeItem* takeChild(int ix);
	void deleteChildren();
	unsigned modelId() const {return m_treeModelId;}

	virtual QVariant data(int role = Qt::UserRole + 1) const;
	void loadChildren();
	bool isChildrenLoaded() const {return m_childrenLoaded;}
	bool isChildrenLoading() const {return m_loadChildrenRqId > 0;}
	std::string shvPath() const;

	void processRpcMessage(const shv::chainpack::RpcMessage &msg);
protected:
	void emitDataChanged();
protected:
	std::string m_nodeId;
	//mutable QMap<qfopcua::AttributeId::Enum, QVariant> m_attribudes;
	bool m_childrenLoaded = false;
	unsigned m_loadChildrenRqId = 0;
	QVector<ShvNodeItem*> m_children;
	unsigned m_treeModelId = 0;
};

class ShvNodeRootItem : public ShvNodeItem
{
	Q_OBJECT

	using Super = ShvNodeItem;
public:
	ShvNodeRootItem(ServerTreeModel *parent);
};
