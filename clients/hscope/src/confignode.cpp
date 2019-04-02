#include "confignode.h"
#include "hnode.h"

#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/core/exception.h>
#include <shv/coreqt/log.h>
#include <shv/iotqt/utils/shvpath.h>

#include <fstream>

#define logConfig() shvCDebug("Config").color(NecroLog::Color::Yellow)

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
	{shv::iotqt::node::RpcValueMapNode::M_SAVE, cp::MetaMethod::Signature::RetVoid, 0, cp::Rpc::GRANT_ADMIN},
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

ConfigNode::ConfigNode(HNode *parent)
	: Super(".config", parent)
{
	shvDebug() << "creating:" << metaObject()->className() << nodeId();
}

size_t ConfigNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()) {
		return meta_methods_root_node.size();
	}
	else {
		return isDir(shv_path)? 2: meta_methods_node.size();
	}
}

const shv::chainpack::MetaMethod *ConfigNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
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

shv::chainpack::RpcValue ConfigNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(method == METH_ORIG_VALUE) {
		return valueOnPath(m_templateValues, shv_path, shv::core::Exception::Throw);
	}
	if(method == METH_RESET_TO_ORIG_VALUE) {
		cp::RpcValue orig_val = valueOnPath(m_templateValues, shv_path, shv::core::Exception::Throw);
		setValueOnPath(shv_path, orig_val);
		return true;
	}
	return Super::callMethod(shv_path, method, params);
}

HNode *ConfigNode::parentHNode()
{
	return qobject_cast<HNode*>(parent());
}

shv::chainpack::RpcValue ConfigNode::loadConfigTemplate(const std::string &file_name)
{
	shvLogFuncFrame() << file_name;
	std::ifstream is(file_name);
	if(is) {
		cp::CponReader rd(is);
		std::string err;
		shv::chainpack::RpcValue rv = rd.read(&err);
		if(err.empty()) {
			const shv::chainpack::RpcValue::Map &map = rv.toMap();
			static const char BASED_ON[] = "basedOn";
			const std::string &based_on = map.value(BASED_ON).toString();
			if(!based_on.empty()) {
				shvDebug() << "based on:" << based_on;
				std::string base_fn = parentHNode()->templatesDir() + '/' + based_on + ".config.cpon";
				shv::chainpack::RpcValue rv2 = loadConfigTemplate(base_fn);
				for (const auto &kv : map) {
					rv2.set(kv.first, kv.second);
				}
				rv = rv2;
			}
			shvDebug() << "return:" << rv.toCpon("\t");
			return rv;
		}
		else {
			shvWarning() << "Cpon parsing error:" << err << "file:" << file_name;
		}
	}
	else {
		shvWarning() << "Cannot open file:" << file_name;
	}
	return shv::chainpack::RpcValue();
}


static cp::RpcValue mergeMaps(const cp::RpcValue &template_val, const cp::RpcValue &user_val)
{
	if(template_val.isMap() && user_val.isMap()) {
		const shv::chainpack::RpcValue::Map &template_map = template_val.toMap();
		const shv::chainpack::RpcValue::Map &user_map = user_val.toMap();
		cp::RpcValue::Map map;
		for(const auto &kv : template_map)
			map[kv.first] = kv.second;
		for(const auto &kv : user_map) {
			if(map.hasKey(kv.first)) {
				map[kv.first] = mergeMaps(map.value(kv.first), kv.second);
			}
			else {
				shvWarning() << "user key:" << kv.first << "not found in template map";
			}
		}
		return cp::RpcValue(map);
	}
	return user_val;
}

static cp::RpcValue diffMaps(const cp::RpcValue &template_vals, const cp::RpcValue &vals)
{
	if(template_vals.isMap() && vals.isMap()) {
		const shv::chainpack::RpcValue::Map &templ_map = template_vals.toMap();
		const shv::chainpack::RpcValue::Map &vals_map = vals.toMap();
		cp::RpcValue::Map map;
		for(const auto &kv : templ_map) {
			cp::RpcValue v = diffMaps(kv.second, vals_map.value(kv.first));
			if(v.isValid())
				map[kv.first] = v;
		}
		return map.empty()? cp::RpcValue(): cp::RpcValue(map);
	}
	if(template_vals == vals)
		return cp::RpcValue();
	return vals;
}

void ConfigNode::loadValues()
{
	Super::loadValues();
	{
		std::string cfg_file = parentHNode()->nodeConfigFilePath();
		logConfig() << "Reading config template file" << cfg_file << "node:" << parentHNode()->shvPath();
		cp::RpcValue val = loadConfigTemplate(cfg_file);
		if(val.isMap()) {
			m_templateValues = val;
		}
		else {
			/// file may not exist
			m_templateValues = cp::RpcValue::Map();
			shvWarning() << parentHNode()->shvPath() << "Cannot open config file" << cfg_file << "for reading!";
		}
	}
	cp::RpcValue new_values = cp::RpcValue();
	{
		std::string cfg_file = parentHNode()->nodeConfigDir() + "/config.user.cpon";
		logConfig() << "Reading config user file" << cfg_file << "node:" << parentHNode()->shvPath();
		std::ifstream is(cfg_file);
		if(is) {
			cp::CponReader rd(is);
			new_values = rd.read();
		}
		else {
			/// file may not exist
			new_values = cp::RpcValue::Map();
			//SHV_EXCEPTION("Cannot open file '" + cfg_file + "' for reading!");
		}
	}
	std::string clone = mergeMaps(m_templateValues, new_values).toChainPack();
	std::string err;
	m_values = cp::RpcValue::fromChainPack(clone, &err);
	if(!err.empty())
		shvWarning() << "clone error:" << err;
}

void ConfigNode::saveValues()
{
	cp::RpcValue new_values = diffMaps(m_templateValues, m_values);
	//shvWarning() << "diff:" << new_values.toPrettyString();
	std::string cfg_file = parentHNode()->nodeConfigDir() + "/config.user.cpon";
	std::ofstream os(cfg_file);
	if(os) {
		cp::CponWriterOptions opts;
		opts.setIndent("\t");
		cp::CponWriter wr(os, opts);
		wr.write(new_values);
		wr.flush();
		emit configSaved();
		return;
	}
	SHV_EXCEPTION("Cannot open file '" + cfg_file + "' for writing!");
}
