#ifndef HNODESNODE_H
#define HNODESNODE_H

#include "hnode.h"

class HNodeNodes : public HNode
{
	Q_OBJECT

	using Super = HNode;
public:
	HNodeNodes(const std::string &node_id, shv::iotqt::node::ShvNode *parent);
public:
	void load() override;
};

#endif // HNODESNODE_H
