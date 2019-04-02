#include "hnodetests.h"
#include "hnodetest.h"

#include <shv/coreqt/log.h>

HNodeTests::HNodeTests(const std::string &node_id, HNode *parent)
	: Super(node_id, parent)
{
	shvDebug() << "creating:" << metaObject()->className() << node_id;
}

void HNodeTests::load()
{
	Super::load();
	for (const std::string &dir : lsConfigDir()) {
		auto *nd = new HNodeTest(dir, this);
		nd->load();
	}
}

