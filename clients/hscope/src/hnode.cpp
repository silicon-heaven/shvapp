#include "hnode.h"
#include "hscopeapp.h"
#include "appclioptions.h"
#include "hnodebroker.h"
#include "hnodeconfig.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/utils/shvpath.h>

#include <QDirIterator>

namespace cp = shv::chainpack;

//===========================================================
// HNode
//===========================================================
HNode::HNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent)
	: Super(node_id, parent)
{
}

std::string HNode::configDir()
{
	HScopeApp *app = HScopeApp::instance();
	return app->cliOptions()->configDir() + '/' + shvPath();
}

void HNode::load()
{
	new HNodeConfig(this);
}

std::vector<std::string> HNode::lsConfigDir()
{
	std::vector<std::string> ret;
	QString node_cfg_dir = QString::fromStdString(configDir());
	shvDebug() << "ls dir:" << node_cfg_dir;
	QDirIterator it(node_cfg_dir, QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Readable, QDirIterator::NoIteratorFlags);
	while (it.hasNext()) {
		it.next();
		shvDebug() << "\t" << it.fileName();
		ret.push_back(it.fileName().toStdString());
	}
	return ret;
}

