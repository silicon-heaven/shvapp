#include "hnodebroker.h"
#include "hnodenodes.h"

#include <shv/coreqt/log.h>

HNodeBroker::HNodeBroker(const std::string &node_id, ShvNode *parent)
	: Super(node_id, parent)
{
	shvInfo() << "creating:" << metaObject()->className() << node_id;
}

void HNodeBroker::load()
{
	Super::load();
	for (const std::string &dir : lsConfigDir()) {
		auto *nd = new HNodeNodes(dir, this);
		nd->load();
	}
}
