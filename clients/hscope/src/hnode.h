#ifndef HNODE_H
#define HNODE_H

#include <shv/iotqt/node/shvnode.h>

class HNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;
public:
	HNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent);

	std::string configDir();
	virtual void load() = 0;
protected:
	std::vector<std::string> lsConfigDir();
};

class HNodeConfigNode : public shv::iotqt::node::RpcValueMapNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::RpcValueMapNode;
public:
	HNodeConfigNode(HNode *parent);

protected:
	HNode* parentHNode();
	void loadValues() override;
	bool saveValues() override;
	shv::chainpack::RpcValue valueOnPath(const StringViewList &shv_path) override;
	void setValueOnPath(const StringViewList &shv_path, const shv::chainpack::RpcValue &val) override;

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod *metaMethod(const StringViewList &shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
protected:
	shv::chainpack::RpcValue m_newValues;
};

#endif // HNODE_H
