#ifndef HBROKERSNODE_H
#define HBROKERSNODE_H

#include "hnode.h"

class HBrokersNode : public HNode
{
	Q_OBJECT

	using Super = HNode;
public:
	HBrokersNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent);
public:
	void load() override;
};

#endif // HBROKERSNODE_H
