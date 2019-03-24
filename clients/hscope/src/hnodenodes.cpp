#include "hnodenodes.h"
#include "hnodenode.h"

#include <shv/coreqt/log.h>

HNodeNodes::HNodeNodes(const std::string &node_id, ShvNode *parent)
	: Super(node_id, parent)
{
	shvInfo() << "creating:" << metaObject()->className() << node_id;
}

void HNodeNodes::load()
{
	Super::load();
	for (const std::string &dir : lsConfigDir()) {
		auto *nd = new HNodeNode(dir, this);
		nd->load();
	}
}
