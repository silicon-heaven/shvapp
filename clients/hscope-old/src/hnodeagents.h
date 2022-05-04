#ifndef HNODESNODE_H
#define HNODESNODE_H

#include "hnode.h"

class HNodeAgents : public HNode
{
	Q_OBJECT

	using Super = HNode;
public:
	HNodeAgents(const std::string &node_id, HNode *parent);
public:
	void load() override;
};

#endif // HNODESNODE_H
