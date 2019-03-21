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
	shv::chainpack::RpcValue loadValues() override;
	bool saveValues(const shv::chainpack::RpcValue &vals) override;
};

#endif // HNODE_H
