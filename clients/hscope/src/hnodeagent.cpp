#include "hnodeagent.h"
#include "hnodetests.h"

#include <shv/coreqt/log.h>

static const char KEY_SHV_PATH[] = "shvPath";

HNodeAgent::HNodeAgent(const std::string &node_id, HNode *parent)
	: Super(node_id, parent)
{
	shvDebug() << "creating:" << metaObject()->className() << node_id;
}

void HNodeAgent::load()
{
	Super::load();
	for (const std::string &dir : lsConfigDir()) {
		auto *nd = new HNodeTests(dir, this);
		nd->load();
	}
}

std::string HNodeAgent::agentShvPath() const
{
	return configValueOnPath(KEY_SHV_PATH).toString();
}

std::string HNodeAgent::templateFileName()
{
	return "agent.config.cpon";
}
