#include "htestnode.h"

#include <shv/coreqt/log.h>

HTestNode::HTestNode(const std::string &node_id, ShvNode *parent)
	: Super(node_id, parent)
{
	shvInfo() << "creating:" << metaObject()->className() << node_id;
}

void HTestNode::load()
{
}

