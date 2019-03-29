#ifndef HBROKERSNODE_H
#define HBROKERSNODE_H

#include "hnode.h"

class HNodeBrokers : public HNode
{
	Q_OBJECT

	using Super = HNode;
public:
	HNodeBrokers(const std::string &node_id, HNode *parent);
public:
	void load() override;
};

#endif // HBROKERSNODE_H
