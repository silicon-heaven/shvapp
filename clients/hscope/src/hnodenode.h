#ifndef HNODENODE_H
#define HNODENODE_H

#include "hnode.h"

class HNodeNode : public HNode
{
	Q_OBJECT

	using Super = HNode;
public:
	HNodeNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent);
public:
	void load() override;
};

#endif // HNODENODE_H
