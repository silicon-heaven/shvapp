#include "hnode.h"
#include "hscopeapp.h"
#include "appclioptions.h"
#include "hbrokernode.h"

#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/coreqt/log.h>
#include <shv/iotqt/utils/shvpath.h>

#include <QDirIterator>

#include <fstream>

namespace cp = shv::chainpack;

//===========================================================
// HNode
//===========================================================
HNode::HNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent)
	: Super(node_id, parent)
{
	new HNodeConfigNode(this);
}

std::string HNode::configDir()
{
	HScopeApp *app = HScopeApp::instance();
	return app->cliOptions()->configDir() + '/' + shvPath();
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

//===========================================================
// HNodeConfigNode
//===========================================================
HNodeConfigNode::HNodeConfigNode(HNode *parent)
	: Super("config", parent)
{

}

HNode *HNodeConfigNode::parentHNode()
{
	return qobject_cast<HNode*>(parent());
}

shv::chainpack::RpcValue HNodeConfigNode::loadValues()
{
	try {
		std::string cfg_file = parentHNode()->configDir() + "/config.cpon";
		std::ifstream is(cfg_file);
		if(is) {
			cp::CponReader rd(is);
			return rd.read();
		}
		else {
			shvError() << "Cannot open file" << cfg_file << "for reading!";
		}
	}
	catch (std::exception &e) {
		shvError() << "Read config file error:" << e.what();
	}
	return cp::RpcValue();
}

bool HNodeConfigNode::saveValues(const shv::chainpack::RpcValue &vals)
{
	try {
		std::string cfg_file = parentHNode()->configDir() + "/config.cpon";
		std::ofstream os(cfg_file);
		if(os) {
			cp::CponWriter wr(os);
			wr.write(vals);
			return true;
		}
		else {
			shvError() << "Cannot open file" << cfg_file << "for reading!";
		}
	}
	catch (std::exception &e) {
		shvError() << "Write config file error:" << e.what();
	}
	return false;
}
