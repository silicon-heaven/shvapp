#include "hnodebrokers.h"
#include "hscopeapp.h"
#include "appclioptions.h"
#include "hnodebroker.h"

#include <shv/coreqt/log.h>
#include <shv/core/utils/shvpath.h>

#include <QDirIterator>

HNodeBrokers::HNodeBrokers(const std::string &node_id, HNode *parent)
	: Super(node_id, parent)
{
	shvDebug() << "creating:" << metaObject()->className() << node_id;
}

void HNodeBrokers::load()
{
	Super::load();
	for (const std::string &dir : lsConfigDir()) {
		auto *nd = new HNodeBroker(dir, this);
		nd->load();
	}
}
