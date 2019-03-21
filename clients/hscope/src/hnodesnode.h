#ifndef HNODESNODE_H
#define HNODESNODE_H

#include "hnode.h"

class HNodesNode : public HNode
{
	Q_OBJECT

	using Super = HNode;
public:
	HNodesNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent);
public:
	void load() override;
};

#endif // HNODESNODE_H
