#include "hnodetest.h"

#include <shv/coreqt/log.h>

HNodeTest::HNodeTest(const std::string &node_id, ShvNode *parent)
	: Super(node_id, parent)
{
	shvInfo() << "creating:" << metaObject()->className() << node_id;
}

void HNodeTest::load()
{
	Super::load();
}

