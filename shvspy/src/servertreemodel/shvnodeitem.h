#pragma once

#include <shv/core/utils.h>

#include <QObject>
#include <QVector>

class ShvBrokerNodeItem;
class ServerTreeModel;

namespace shv { namespace chainpack { class RpcMessage; } }

class ShvMetaMethod
{
	SHV_FIELD_IMPL(std::string, n, N, ame)
	SHV_FIELD_IMPL(std::string, p, P, arams)
	SHV_FIELD_IMPL(std::string, r, R, esult)
};

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
	std::string shvPath() const;

	const QVector<ShvMetaMethod>& methods() const {return m_methods;}

	void loadChildren();
	bool isChildrenLoaded() const {return m_childrenLoaded;}
	bool isChildrenLoading() const {return m_loadChildrenRqId > 0;}

	bool checkMethodsLoaded();
	void loadMethods();
	bool isMethodsLoaded() const {return m_methodsLoaded;}
	bool isMethodsLoading() const {return m_loadMethodsRqId > 0;}
	Q_SIGNAL void methodsLoaded();

	void processRpcMessage(const shv::chainpack::RpcMessage &msg);
protected:
	void emitDataChanged();
protected:
	std::string m_nodeId;
	//mutable QMap<qfopcua::AttributeId::Enum, QVariant> m_attribudes;
	bool m_childrenLoaded = false;
	unsigned m_loadChildrenRqId = 0;
	QVector<ShvNodeItem*> m_children;
	QVector<ShvMetaMethod> m_methods;
	bool m_methodsLoaded = false;
	unsigned m_loadMethodsRqId = 0;
	unsigned m_treeModelId = 0;
};

class ShvNodeRootItem : public ShvNodeItem
{
	Q_OBJECT

	using Super = ShvNodeItem;
public:
	ShvNodeRootItem(ServerTreeModel *parent);
};
