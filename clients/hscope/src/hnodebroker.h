#ifndef HBROKERNODE_H
#define HBROKERNODE_H

#include "hnode.h"

class HNodeBroker : public HNode
{
	Q_OBJECT

	using Super = HNode;
public:
	HNodeBroker(const std::string &node_id, shv::iotqt::node::ShvNode *parent);
public:
	void load() override;
};

#endif // HBROKERNODE_H
