#include "hbrokernode.h"
#include "hnodesnode.h"

#include <shv/coreqt/log.h>

HBrokerNode::HBrokerNode(const std::string &node_id, ShvNode *parent)
	: Super(node_id, parent)
{
	shvInfo() << "creating:" << metaObject()->className() << node_id;
}

void HBrokerNode::load()
{
	for (const std::string &dir : lsConfigDir()) {
		auto *nd = new HNodesNode(dir, this);
		nd->load();
	}
}
