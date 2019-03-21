#include "hbrokersnode.h"
#include "hscopeapp.h"
#include "appclioptions.h"
#include "hbrokernode.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/utils/shvpath.h>

#include <QDirIterator>

HBrokersNode::HBrokersNode(const std::string &node_id, ShvNode *parent)
	: Super(node_id, parent)
{
	shvInfo() << "creating:" << metaObject()->className() << node_id;
}

void HBrokersNode::load()
{
	for (const std::string &dir : lsConfigDir()) {
		auto *nd = new HBrokerNode(dir, this);
		nd->load();
	}
}
