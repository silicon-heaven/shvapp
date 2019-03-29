#ifndef HNODENODE_H
#define HNODENODE_H

#include "hnode.h"

class HNodeAgent : public HNode
{
	Q_OBJECT

	using Super = HNode;
public:
	HNodeAgent(const std::string &node_id, HNode *parent);
public:
	void load() override;

	std::string agentShvPath() const;
	std::string templateFileName() override;
};

#endif // HNODENODE_H
