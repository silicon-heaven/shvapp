#include "hnodeagents.h"
#include "hnodeagent.h"

#include <shv/coreqt/log.h>

HNodeAgents::HNodeAgents(const std::string &node_id, HNode *parent)
	: Super(node_id, parent)
{
	shvDebug() << "creating:" << metaObject()->className() << node_id;
}

void HNodeAgents::load()
{
	Super::load();
	for (const std::string &dir : lsConfigDir()) {
		auto *nd = new HNodeAgent(dir, this);
		nd->load();
	}
}
