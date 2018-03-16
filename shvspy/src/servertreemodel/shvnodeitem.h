#pragma once

#include <QStandardItem>

class ShvBrokerNodeItem;

namespace shv { namespace chainpack { class RpcMessage; } }

class ShvNodeItem : public QStandardItem
{
private:
	typedef QStandardItem Super;
public:
	ShvNodeItem(const std::string &ndid);
	~ShvNodeItem() Q_DECL_OVERRIDE;

	const std::string& nodeId() const {return m_nodeId;}
	ShvBrokerNodeItem* serverNode() const;

	QVariant data(int role = Qt::UserRole + 1) const Q_DECL_OVERRIDE;
	void loadChildren();
	bool isChildrenLoaded() const {return m_childrenLoaded;}
	bool isChildrenLoading() const {return m_loadChildrenRqId > 0;}
	std::string shvPath() const;

	void processRpcMessage(const shv::chainpack::RpcMessage &msg);
protected:
	//QVariant attribute(qfopcua::AttributeId::Enum attr_id) const;
protected:
	std::string m_nodeId;
	//mutable QMap<qfopcua::AttributeId::Enum, QVariant> m_attribudes;
	bool m_childrenLoaded = false;
	unsigned m_loadChildrenRqId = 0;
};

