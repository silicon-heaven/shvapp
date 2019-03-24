#include "hnodeconfig.h"
#include "hnode.h"

#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/core/exception.h>
#include <shv/coreqt/log.h>
#include <shv/iotqt/utils/shvpath.h>

#include <fstream>

namespace cp = shv::chainpack;

//===========================================================
// HNodeConfigNode
//===========================================================
static const char METH_ORIG_VALUE[] = "origValue";
static const char METH_RESET_TO_ORIG_VALUE[] = "resetValue";

static std::vector<cp::MetaMethod> meta_methods_root_node {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_CONFIG},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_CONFIG},
	{shv::iotqt::node::RpcValueMapNode::M_LOAD, cp::MetaMethod::Signature::RetVoid, 0, cp::Rpc::GRANT_SERVICE},
	{shv::iotqt::node::RpcValueMapNode::M_COMMIT, cp::MetaMethod::Signature::RetVoid, 0, cp::Rpc::GRANT_ADMIN},
};

static std::vector<cp::MetaMethod> meta_methods_node {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_CONFIG},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_CONFIG},
	{cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::GRANT_CONFIG},
	{cp::Rpc::METH_SET, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsSetter, cp::Rpc::GRANT_DEVEL},
	{METH_ORIG_VALUE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::GRANT_READ},
	{METH_RESET_TO_ORIG_VALUE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::GRANT_WRITE},
};

HNodeConfig::HNodeConfig(HNode *parent)
	: Super("config", parent)
{
	shvInfo() << "creating:" << metaObject()->className() << nodeId();
}

size_t HNodeConfig::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()) {
		return meta_methods_root_node.size();
	}
	else {
		return isDir(shv_path)? 2: meta_methods_node.size();
	}
}

const shv::chainpack::MetaMethod *HNodeConfig::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	size_t size;
	const std::vector<shv::chainpack::MetaMethod> &methods = shv_path.empty()? meta_methods_root_node: meta_methods_node;
	if(shv_path.empty()) {
		size = meta_methods_root_node.size();
	}
	else {
		size = isDir(shv_path)? 2: meta_methods_node.size();
	}
	if(size <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(size));
	return &(methods[ix]);
}

shv::chainpack::RpcValue HNodeConfig::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(method == METH_ORIG_VALUE) {
		return Super::valueOnPath(shv_path);
	}
	if(method == METH_RESET_TO_ORIG_VALUE) {
		setValueOnPath(shv_path, cp::RpcValue());
		return true;
	}
	return Super::callMethod(shv_path, method, params);
}

HNode *HNodeConfig::parentHNode()
{
	return qobject_cast<HNode*>(parent());
}

void HNodeConfig::loadValues()
{
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
			/// file may not exist
			m_values = cp::RpcValue::Map();
			//SHV_EXCEPTION("Cannot open file '" + cfg_file + "' for reading!");
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
		else {
			/// file may not exist
			m_newValues = cp::RpcValue::Map();
			//SHV_EXCEPTION("Cannot open file '" + cfg_file + "' for reading!");
		}
	}
	Super::loadValues();
}

bool HNodeConfig::saveValues()
{
	std::string cfg_file = parentHNode()->configDir() + "/config.user.cpon";
	std::ofstream os(cfg_file);
	if(os) {
		cp::CponWriter wr(os);
		wr.write(m_newValues);
		return true;
	}
	SHV_EXCEPTION("Cannot open file '" + cfg_file + "' for writing!");
}

shv::chainpack::RpcValue HNodeConfig::valueOnPath(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
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

void HNodeConfig::setValueOnPath(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const shv::chainpack::RpcValue &val)
{
	/*
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
	*/
	setValueOnPathR(shv_path, val);
}

void HNodeConfig::setValueOnPathR(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const shv::chainpack::RpcValue &val)
{
	values();
	setValueOnPath_helper(shv_path, 0, m_newValues, val);
}

void HNodeConfig::setValueOnPath_helper(const shv::iotqt::node::ShvNode::StringViewList &path, size_t key_ix, shv::chainpack::RpcValue parent_map, const shv::chainpack::RpcValue &val)
{
	shvLogFuncFrame() << "path:" << path.join('/') << "ix:" << key_ix << "parent:" << parent_map.toCpon() << "val:" << val.toPrettyString();
	if(key_ix == path.size() - 1) {
		if(!parent_map.isMap())
			SHV_EXCEPTION("Items should be maps, path: " + path.join('/') + " current dir: " + path.at(key_ix).toString());
		parent_map.set(path.at(key_ix).toString(), val);
	}
	else {
		cp::RpcValue map = parent_map.at(path.at(key_ix).toString());
		if(!map.isValid() && val.isValid()) {
			/// create missing dir
			map = cp::RpcValue::Map();
			parent_map.set(path.at(key_ix).toString(), map);
		}
		if(map.isMap()) {
			setValueOnPath_helper(path, key_ix+1, map, val);
			if(map.count() == 0 && !val.isValid()) {
				/// remove empty dir
				parent_map.set(path.at(key_ix).toString(), val);
			}
		}
		else if(map.isValid()) {
			SHV_EXCEPTION("Items should be maps, path: " + path.join('/') + " current dir: " + path.at(key_ix).toString());
		}
		else {
			SHV_EXCEPTION("Invalid path: " + path.join('/') + " current dir: " + path.at(key_ix).toString());
		}
	}
}
