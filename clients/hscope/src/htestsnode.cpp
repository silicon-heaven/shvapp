#include "htestsnode.h"
#include "htestnode.h"

#include <shv/coreqt/log.h>

HTestsNode::HTestsNode(const std::string &node_id, ShvNode *parent)
	: Super(node_id, parent)
{
	shvInfo() << "creating:" << metaObject()->className() << node_id;
}

void HTestsNode::load()
{
	for (const std::string &dir : lsConfigDir()) {
		auto *nd = new HTestNode(dir, this);
		nd->load();
	}
}

