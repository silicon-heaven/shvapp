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
static const char METH_ORIG_VALUE[] = "origValue";
static std::vector<cp::MetaMethod> meta_methods_config_node {
	{METH_ORIG_VALUE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::GRANT_READ},
};

HNodeConfigNode::HNodeConfigNode(HNode *parent)
	: Super("config", parent)
{

}

HNode *HNodeConfigNode::parentHNode()
{
	return qobject_cast<HNode*>(parent());
}

void HNodeConfigNode::loadValues()
{
	try {
		Super::loadValues();
		m_values = cp::RpcValue();
		{
			std::string cfg_file = parentHNode()->configDir() + "/config.cpon";
			std::ifstream is(cfg_file);
			if(is) {
				cp::CponReader rd(is);
				m_values = rd.read();
			}
			else {
				shvError() << "Cannot open file" << cfg_file << "for reading!";
			}
		}
		m_newValues = cp::RpcValue();
		{
			std::string cfg_file = parentHNode()->configDir() + "/config.user.cpon";
			std::ifstream is(cfg_file);
			if(is) {
				cp::CponReader rd(is);
				m_newValues = rd.read();
			}
		}
		Super::loadValues();
	}
	catch (std::exception &e) {
		shvError() << "Read config file error:" << e.what();
	}
}

bool HNodeConfigNode::saveValues()
{
	try {
		if(!m_newValues.toMap().empty()) {
			std::string cfg_file = parentHNode()->configDir() + "/config.user.cpon";
			std::ofstream os(cfg_file);
			if(os) {
				cp::CponWriter wr(os);
				wr.write(m_newValues);
				return true;
			}
			else {
				shvError() << "Cannot open file" << cfg_file << "for writing!";
			}
		}
	}
	catch (std::exception &e) {
		shvError() << "Write config file error:" << e.what();
	}
	return false;
}

shv::chainpack::RpcValue HNodeConfigNode::valueOnPath(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	shv::chainpack::RpcValue orig_val = Super::valueOnPath(shv_path);
	if(orig_val.isMap()) {
		// take directory structure from orig values
		return orig_val;
	}
	shv::chainpack::RpcValue v = m_newValues;
	for(const auto & dir : shv_path) {
		const shv::chainpack::RpcValue::Map &m = v.toMap();
		v = m.value(dir.toString());
		if(!v.isValid())
			break;
	}
	if(v.isValid())
		return v;
	return orig_val;
}

void HNodeConfigNode::setValueOnPath(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const shv::chainpack::RpcValue &val)
{
	values();
	if(shv_path.empty())
		SHV_EXCEPTION("Empty path");
	if(!m_newValues.isMap())
		m_newValues = cp::RpcValue::Map();
	shv::chainpack::RpcValue v = m_newValues;
	for (size_t i = 0; i < shv_path.size()-1; ++i) {
		std::string dir = shv_path.at(i).toString();
		cp::RpcValue v2 = v.at(dir);
		if(v2.isValid() && !v2.isMap())
			SHV_EXCEPTION("Cannot change leaf to dir, path: " + shv_path.join('/'));
		if(!v2.isMap()) {
			v2 = cp::RpcValue::Map();
			v.set(dir, v2);
		}
		v = v2;
	}
	v.set(shv_path.at(shv_path.size() - 1).toString(), val);
}

size_t HNodeConfigNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(!shv_path.empty() && !isDir(shv_path))
		return Super::methodCount(shv_path) + 1;
	return Super::methodCount(shv_path);
}

const shv::chainpack::MetaMethod *HNodeConfigNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	if(!shv_path.empty() && !isDir(shv_path)) {
		size_t scnt = Super::methodCount(shv_path);
		if(ix == scnt)
			return &(meta_methods_config_node[0]);
	}
	return Super::metaMethod(shv_path, ix);
}

shv::chainpack::RpcValue HNodeConfigNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(method == METH_ORIG_VALUE) {
		return Super::valueOnPath(shv_path);
	}
	return Super::callMethod(shv_path, method, params);
}
