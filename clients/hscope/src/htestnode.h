#ifndef HTESTNODE_H
#define HTESTNODE_H

#include "hnode.h"

class HTestNode : public HNode
{
	Q_OBJECT

	using Super = HNode;
public:
	HTestNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent);
public:
	void load() override;
};

#endif // HTESTNODE_H
