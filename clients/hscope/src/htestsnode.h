#ifndef HTESTSNODE_H
#define HTESTSNODE_H

#include "hnode.h"

class HTestsNode : public HNode
{
	Q_OBJECT

	using Super = HNode;
public:
	HTestsNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent);
public:
	void load() override;
};

#endif // HTESTSNODE_H
