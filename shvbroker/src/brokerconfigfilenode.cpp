#include "brokerconfigfilenode.h"
#include "brokerapp.h"

#include <shv/iotqt/utils/shvpath.h>
#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpc.h>
#include <shv/core/string.h>
#include <shv/core/log.h>

namespace cp = shv::chainpack;

//========================================================
// EtcAclNode
//========================================================
static std::vector<cp::MetaMethod> meta_methods_acl {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::GRANT_CONFIG},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::GRANT_CONFIG},
};

static const char M_ADD_USER[] = "addUser";
static const char M_DEL_USER[] = "delUser";

static const char M_ADD_GRANT[] = "addGrant";
static const char M_EDIT_GRANT[] = "editGrant";
static const char M_DEL_GRANT[] = "delGrant";

static std::vector<cp::MetaMethod> meta_methods_acl_users {
	{M_ADD_USER, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_CONFIG},
	{M_DEL_USER, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_CONFIG}
};

static std::vector<cp::MetaMethod> meta_methods_acl_grants {
	{M_ADD_GRANT, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_CONFIG},
	{M_EDIT_GRANT, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_CONFIG},
	{M_DEL_GRANT, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_CONFIG}
};

EtcAclNode::EtcAclNode(shv::iotqt::node::ShvNode *parent)
	: Super("acl", &meta_methods_acl, parent)
{
	{
		auto *nd = new BrokerConfigFileNode("fstab", this);
		connect(BrokerApp::instance(), &BrokerApp::configReloaded, nd, &shv::iotqt::node::RpcValueMapNode::clearValuesCache);
	}
	{
		auto *nd = new BrokerUsersConfigFileNode("users", this);
		connect(BrokerApp::instance(), &BrokerApp::configReloaded, nd, &shv::iotqt::node::RpcValueMapNode::clearValuesCache);
	}
	{
		auto *nd = new BrokerGrantsConfigFileNode("grants", this);
		connect(BrokerApp::instance(), &BrokerApp::configReloaded, nd, &shv::iotqt::node::RpcValueMapNode::clearValuesCache);
	}
	{
		auto *nd = new AclPathsConfigFileNode(this);
		connect(BrokerApp::instance(), &BrokerApp::configReloaded, nd, &shv::iotqt::node::RpcValueMapNode::clearValuesCache);
	}
}

enum AclAccessLevel {AclUserView = cp::MetaMethod::AccessLevel::Service, AclUserAdmin = cp::MetaMethod::AccessLevel::Admin};

BrokerConfigFileNode::BrokerConfigFileNode(const std::string &config_name, shv::iotqt::node::ShvNode *parent)
	: Super(config_name, parent)
{
}

void BrokerConfigFileNode::loadValues()
{
	m_values = BrokerApp::instance()->aclConfig(nodeId(), !shv::core::Exception::Throw);
	Super::loadValues();
}

void BrokerConfigFileNode::saveValues()
{
	BrokerApp::instance()->setAclConfig(nodeId(), m_values, shv::core::Exception::Throw);
}

//========================================================
// BrokerGrantsConfigFileNode
//========================================================
BrokerGrantsConfigFileNode::BrokerGrantsConfigFileNode(const std::string &config_name, shv::iotqt::node::ShvNode *parent)
	: Super(config_name, parent)
{
}

size_t BrokerGrantsConfigFileNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	return (shv_path.empty()) ? meta_methods_acl_grants.size() + Super::methodCount(shv_path) : Super::methodCount(shv_path);
}

const shv::chainpack::MetaMethod *BrokerGrantsConfigFileNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty()) {
		size_t all_method_count = meta_methods_acl_grants.size() + Super::methodCount(shv_path);

		if (ix < meta_methods_acl_grants.size())
			return &(meta_methods_acl_grants[ix]);
		else if (ix < all_method_count)
			return Super::metaMethod(shv_path, ix - meta_methods_acl_grants.size());
		else
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(all_method_count));
	}

	return Super::metaMethod(shv_path, ix);
}

shv::chainpack::RpcValue BrokerGrantsConfigFileNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == M_ADD_GRANT) {
			return addGrant(params);
		}
		else if(method == M_EDIT_GRANT) {
			return editGrant(params);
		}
		else if(method == M_DEL_GRANT) {
			return delGrant(params);
		}
	}

	return Super::callMethod(shv_path, method, params);
}

bool BrokerGrantsConfigFileNode::addGrant(const shv::chainpack::RpcValue &params)
{
	if (!params.isMap() || !params.toMap().hasKey("grantName")){
		SHV_EXCEPTION("Invalid parameters format. Params must be a map and it must contains key: grantName.");
	}

	cp::RpcValue::Map map = params.toMap();
	std::string grant_name = map.value("grantName").toStdString();

	cp::RpcValue::Map grants_config = values().toMap();

	if (grants_config.hasKey(grant_name)){
		SHV_EXCEPTION("Grant " + grant_name + " already exists");
	}

	cp::RpcValue::Map new_grant;

	if (map.hasKey("grants")){
		const cp::RpcValue grants = map.value("grants");
		if (grants.isList()){
			new_grant["grants"] = grants;
		}
	}
	if (map.hasKey("weight")){
		const cp::RpcValue weight = map.value("weight");
		if (weight.isInt()){
			new_grant["weight"] = weight;
		}
	}

	m_values.set(grant_name, new_grant);
	commitChanges();

	return true;
}

bool BrokerGrantsConfigFileNode::editGrant(const shv::chainpack::RpcValue &params)
{
	if (!params.isMap() || !params.toMap().hasKey("grantName")){
		SHV_EXCEPTION("Invalid parameters format. Params must be a RpcValue::Map and it must contains key: grantName.");
	}

	cp::RpcValue::Map params_map = params.toMap();
	std::string grant_name = params_map.value("grantName").toStdString();

	cp::RpcValue::Map grants_config = values().toMap();

	if (!grants_config.hasKey(grant_name)){
		SHV_EXCEPTION("Grant " + grant_name + " does not exist.");
	}

	cp::RpcValue::Map new_grant;

	if (params_map.hasKey("grants")){
		const cp::RpcValue grants = params_map.value("grants");
		if (grants.isList()){
			new_grant["grants"] = grants;
		}
		else{
			SHV_EXCEPTION("Invalid parameters format. Param grants must be a RpcValue::List.");
		}
	}
	if (params_map.hasKey("weight")){
		const cp::RpcValue weight = params_map.value("weight");
		if (weight.isInt()){
			new_grant["weight"] = weight;
		}
		else{
			SHV_EXCEPTION("Invalid parameters format. Param weight must be a RpcValue::Int.");
		}
	}

	m_values.set(grant_name, new_grant);
	commitChanges();

	return true;
}

bool BrokerGrantsConfigFileNode::delGrant(const shv::chainpack::RpcValue &params)
{
	if (!params.isString() || params.toString().empty()){
		SHV_EXCEPTION("Invalid parameters format. Param must be non empty string.");
	}

	std::string grant_name = params.toString();
	const cp::RpcValue::Map &grants_config = values().toMap();

	if (!grants_config.hasKey(grant_name)){
		SHV_EXCEPTION("Grant " + grant_name + " does not exist.");
	}

	m_values.set(grant_name, cp::RpcValue());
	commitChanges();

	return true;
}

//========================================================
// BrokerUsersConfigFileNode
//========================================================
BrokerUsersConfigFileNode::BrokerUsersConfigFileNode(const std::string &config_name, shv::iotqt::node::ShvNode *parent)
	: Super(config_name, parent)
{
}

size_t BrokerUsersConfigFileNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	return (shv_path.empty()) ? meta_methods_acl_users.size() + Super::methodCount(shv_path) : Super::methodCount(shv_path);
}

const shv::chainpack::MetaMethod *BrokerUsersConfigFileNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty()) {
		size_t all_method_count = meta_methods_acl_users.size() + Super::methodCount(shv_path);

		if (ix < meta_methods_acl_users.size())
			return &(meta_methods_acl_users[ix]);
		else if (ix < all_method_count)
			return Super::metaMethod(shv_path, ix - meta_methods_acl_users.size());
		else
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(all_method_count));
	}

	return Super::metaMethod(shv_path, ix);
}

shv::chainpack::RpcValue BrokerUsersConfigFileNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == M_ADD_USER) {
			return addUser(params);
		}
		else if(method == M_DEL_USER) {
			return delUser(params);
		}
	}

	return Super::callMethod(shv_path, method, params);
}

bool BrokerUsersConfigFileNode::addUser(const shv::chainpack::RpcValue &params)
{
	if (!params.isMap() || !params.toMap().hasKey("user") || !params.toMap().hasKey("password")){
		SHV_EXCEPTION("Invalid parameters format. Params must be a map and it must contains keys: user, password.");
	}

	cp::RpcValue::Map map = params.toMap();
	std::string user_name = map.value("user").toStdString();

	cp::RpcValue::Map users_config = values().toMap();

	if (users_config.hasKey(user_name)){
		SHV_EXCEPTION("User " + user_name + " already exists");
	}

	cp::RpcValue::Map user;
	const cp::RpcValue grants = map.value("grants");
	user["grants"] = (grants.isList()) ? grants.toList() : cp::RpcValue::List();
	user["password"] = map.value("password").toStdString();
	user["passwordFormat"] = "sha1";

	m_values.set(user_name, user);
	commitChanges();

	return true;
}

bool BrokerUsersConfigFileNode::delUser(const shv::chainpack::RpcValue &params)
{
	if (!params.isString() || params.toString().empty()){
		SHV_EXCEPTION("Invalid parameters format. Param must be non empty string.");
	}

	std::string user_name = params.toString();
	const cp::RpcValue::Map &users_config = values().toMap();

	if (!users_config.hasKey(user_name)){
		SHV_EXCEPTION("User " + user_name + " does not exist.");
	}

	m_values.set(user_name, cp::RpcValue());
	commitChanges();

	return true;
}

//========================================================
// AclPathsConfigFileNode
//========================================================

shv::iotqt::node::ShvNode::StringList AclPathsConfigFileNode::childNames(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.size() == 1) {
		std::vector<std::string> keys = values().at(shv_path[0].toString()).toMap().keys();
		for (size_t i = 0; i < keys.size(); ++i)
			keys[i] = shv::iotqt::utils::ShvPath::SHV_PATH_QUOTE + keys[i] + shv::iotqt::utils::ShvPath::SHV_PATH_QUOTE;
		return keys;

	}
	else {
		return Super::childNames(shv_path);
	}
}

shv::chainpack::RpcValue AclPathsConfigFileNode::valueOnPath(const shv::iotqt::node::ShvNode::StringViewList &shv_path, bool throw_exc)
{
	//shvInfo() << "valueOnPath:" << shv_path.join('/');
	shv::chainpack::RpcValue v = values();
	for (size_t i = 0; i < shv_path.size(); ++i) {
		shv::iotqt::node::ShvNode::StringView dir = shv_path[i];
		if(i == 1)
			dir = dir.mid(1, dir.length() - 2);
		const shv::chainpack::RpcValue::Map &m = v.toMap();
		std::string key = dir.toString();
		v = m.value(key);
		//shvInfo() << "\t i:" << i << "key:" << key << "val:" << v.toCpon();
		if(!v.isValid()) {
			if(throw_exc)
				SHV_EXCEPTION("Invalid path: " + shv_path.join('/'));
			return v;
		}
	}
	return v;
}

