#ifndef HTESTNODE_H
#define HTESTNODE_H

#include "hnode.h"

class HNodeTest : public HNode
{
	Q_OBJECT

	using Super = HNode;
public:
	HNodeTest(const std::string &node_id, shv::iotqt::node::ShvNode *parent);
public:
	void load() override;
};

#endif // HTESTNODE_H
