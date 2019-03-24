#ifndef HNODE_H
#define HNODE_H

#include <shv/iotqt/node/shvnode.h>

class HNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;
public:
	HNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent);

	std::string configDir();
	virtual void load();
protected:
	std::vector<std::string> lsConfigDir();
};

#endif // HNODE_H
