#include "hnodenode.h"
#include "htestsnode.h"

#include <shv/coreqt/log.h>

HNodeNode::HNodeNode(const std::string &node_id, ShvNode *parent)
	: Super(node_id, parent)
{
	shvInfo() << "creating:" << metaObject()->className() << node_id;
}

void HNodeNode::load()
{
	for (const std::string &dir : lsConfigDir()) {
		auto *nd = new HTestsNode(dir, this);
		nd->load();
	}
}
