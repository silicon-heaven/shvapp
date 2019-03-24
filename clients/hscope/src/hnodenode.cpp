#include "hnodenode.h"
#include "hnodetests.h"

#include <shv/coreqt/log.h>

HNodeNode::HNodeNode(const std::string &node_id, ShvNode *parent)
	: Super(node_id, parent)
{
	shvInfo() << "creating:" << metaObject()->className() << node_id;
}

void HNodeNode::load()
{
	Super::load();
	for (const std::string &dir : lsConfigDir()) {
		auto *nd = new HNodeTests(dir, this);
		nd->load();
	}
}
