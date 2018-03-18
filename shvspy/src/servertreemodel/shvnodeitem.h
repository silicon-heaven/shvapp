#pragma once

#include <shv/core/utils.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpcvalue.h>

#include <QObject>
#include <QVector>

class ShvBrokerNodeItem;
class ServerTreeModel;

//namespace shv { namespace chainpack { class RpcMessage; class RpcResponse; } }

struct ShvMetaMethod
{
	std::string method;
	shv::chainpack::RpcValue params;
	shv::chainpack::RpcResponse response;
	unsigned rpcRequestId = 0;
	/*
	SHV_FIELD_IMPL(std::string, m, M, ethodName)
	SHV_FIELD_IMPL(std::string, p, P, arams)
	SHV_FIELD_IMPL(std::string, r, R, esult)
	SHV_FIELD_IMPL(std::string, e, E, rror)
	SHV_FIELD_IMPL2(unsigned, r, R, pcRequestId, 0)
	*/
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
	void setMethodParams(int method_ix, const shv::chainpack::RpcValue &params);
	unsigned callMethod(int method_ix);

	void loadChildren();
	bool isChildrenLoaded() const {return m_childrenLoaded;}
	bool isChildrenLoading() const {return m_loadChildrenRqId > 0;}

	bool checkMethodsLoaded();
	void loadMethods();
	bool isMethodsLoaded() const {return m_methodsLoaded;}
	bool isMethodsLoading() const {return m_loadMethodsRqId > 0;}
	Q_SIGNAL void methodsLoaded();

	Q_SIGNAL void rpcMethodCallFinished(int method_ix);

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