#include "brclabusersnode.h"
#include "shv/chainpack/rpcvalue.h"
#include "shvbrclabproviderapp.h"

#include <shv/core/log.h>
#include <shv/core/exception.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/cponreader.h>

#include <QFile>
#include <fstream>

namespace cp = shv::chainpack;

static const char M_ADD_USER[] = "addUser";
static const char M_DEL_USER[] = "delUser";
static const char M_CHANGE_USER_PASSWORD[] = "changeUserPassword";
static const char M_GET_USER_GRANTS[] = "getUserGrants";

static std::vector<cp::MetaMethod> meta_methods {
	{M_ADD_USER, cp::MetaMethod::Flag::None, "RpcValue", "RpcValue", cp::AccessLevel::Config},
	{M_DEL_USER, cp::MetaMethod::Flag::None, "RpcValue", "RpcValue", cp::AccessLevel::Config},
	{M_CHANGE_USER_PASSWORD, cp::MetaMethod::Flag::None, "RpcValue", "RpcValue", cp::AccessLevel::Write},
	{M_GET_USER_GRANTS, cp::MetaMethod::Flag::None, "RpcValue", "RpcValue", cp::AccessLevel::Write}
};

BrclabUsersNode::BrclabUsersNode(const std::string &node_id, const std::string &fn, ShvNode *parent)
	: Super(node_id, parent)
{
	m_usersConfigFileName = fn;
	loadValues();
}

size_t BrclabUsersNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	return (shv_path.empty()) ? meta_methods.size() + Super::methodCount(shv_path) : Super::methodCount(shv_path);
}

const shv::chainpack::MetaMethod *BrclabUsersNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty()) {
		size_t all_method_count = meta_methods.size() + Super::methodCount(shv_path);

		if (ix < meta_methods.size())
			return &(meta_methods[ix]);
		else if (ix < all_method_count)
			return Super::metaMethod(shv_path, ix - meta_methods.size());
		else
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(all_method_count));
	}

	return Super::metaMethod(shv_path, ix);
}

shv::chainpack::RpcValue BrclabUsersNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if(shv_path.empty()) {
		if(method == M_ADD_USER) {
			return addUser(params);
		}
		else if(method == M_DEL_USER) {
			return delUser(params);
		}
		else if(method == M_CHANGE_USER_PASSWORD) {
			return changePassword(params);
		}
		else if(method == M_GET_USER_GRANTS) {
			return getUserGrants(params);
		}
	}

	return Super::callMethod(shv_path, method, params, user_id);
}

void BrclabUsersNode::loadValues()
{
	if (!QFile::exists(QString::fromStdString(m_usersConfigFileName))){
		setUsersConfig(cp::RpcValue::Map());
	}

	std::ifstream ifs(m_usersConfigFileName);
	ifs.exceptions( std::ifstream::failbit | std::ifstream::badbit );

	if (!ifs.good()) {
		SHV_EXCEPTION("Cannot open file " + m_usersConfigFileName +  " for reading!");
	}

	cp::CponReader rd(ifs);
	rd.read(m_values);

	if (!m_values.isMap()){
		SHV_EXCEPTION("Config file " + m_usersConfigFileName + " must be a RpcValue::Map!");
	}

	m_valuesLoaded = true;
}

void BrclabUsersNode::saveValues()
{
	setUsersConfig(m_values);
}

void BrclabUsersNode::setUsersConfig(const shv::chainpack::RpcValue &data)
{
	if(data.isMap()) {
		std::ofstream ofs(m_usersConfigFileName, std::ios::binary | std::ios::trunc);
		if (!ofs) {
			SHV_EXCEPTION("Cannot open config file " + m_usersConfigFileName + " for writing");
		}

		shv::chainpack::CponWriterOptions opts;
		opts.setIndent("  ");
		shv::chainpack::CponWriter wr(ofs, opts);
		wr << data;
	}
	else{
		SHV_EXCEPTION("Config file must be a RpcValue::Map, config name: " + m_usersConfigFileName);
	}

	loadValues();
}

const shv::chainpack::RpcValue &BrclabUsersNode::usersConfig()
{
	if (!m_values.isMap() || !m_valuesLoaded){
		loadValues();
	}

	return m_values;
}

bool BrclabUsersNode::addUser(const cp::RpcValue &params)
{
	if (!params.isMap() || !params.asMap().hasKey("user") || !params.asMap().hasKey("password")){
		SHV_EXCEPTION("Invalid parameters format. Params must be a RpcValue::Map and it must contains keys: user, password.");
	}

	cp::RpcValue::Map map = params.asMap();
	std::string user_name = map.value("user").toStdString();

	cp::RpcValue::Map users_config = usersConfig().asMap();

	if (users_config.hasKey(user_name)){
		SHV_EXCEPTION("User " + user_name + " already exists");
	}

	cp::RpcValue::Map user;
	const cp::RpcValue grants = map.value("grants");
	user["grants"] = (grants.isList()) ? grants.asList() : cp::RpcValue::List();
	user["password"] = map.value("password").toStdString();
	users_config[user_name] = user;

	setUsersConfig(users_config);
	return true;
}

bool BrclabUsersNode::delUser(const shv::chainpack::RpcValue &params)
{
	if (!params.isString() || params.asString().empty()){
		SHV_EXCEPTION("Invalid parameters format. Param must be non empty RpcValue::String.");
	}

	std::string user_name = params.toString();
	const cp::RpcValue::Map &users_config = usersConfig().asMap();

	if (!users_config.hasKey(user_name)){
		SHV_EXCEPTION("User " + user_name + " does not exist.");
	}

	m_values.set(user_name, cp::RpcValue());
	setUsersConfig(m_values);
	return true;
}

bool BrclabUsersNode::changePassword(const cp::RpcValue &params)
{
	if (!params.isMap() || !params.asMap().hasKey("user") || !params.asMap().hasKey("newPassword")){
		SHV_EXCEPTION("Invalid parameters format. Params must be a RpcValue::Map and it must contains keys: user, newPassword.");
	}

	cp::RpcValue::Map map = params.asMap();
	std::string user_name = map.value("user").toStdString();
	std::string new_password_sha1 = map.value("newPassword").toStdString();

	cp::RpcValue::Map users_config = usersConfig().asMap();

	if (users_config.hasKey(user_name)){
		cp::RpcValue::Map user = users_config.at(user_name).asMap();
		user["password"] = new_password_sha1;
		users_config.at(user_name) = user;

		setUsersConfig(users_config);
	}
	else{
		SHV_EXCEPTION("User " + user_name + " does not exist.");
	}

	return true;
}

shv::chainpack::RpcValue BrclabUsersNode::getUserGrants(const cp::RpcValue &params)
{
	if (!params.isMap() || !params.asMap().hasKey("user") || !params.asMap().hasKey("password")){
		SHV_EXCEPTION("Invalid parameters format. Params must be a RpcValue::Map and it must contains keys: user, password.");
	}

	cp::RpcValue::Map map = params.asMap();
	std::string user_name = map.value("user").toStdString();
	std::string password_sha1 = map.value("password").toStdString();

	cp::RpcValue users_config = usersConfig();

	for(const auto &kv :users_config.asMap()) {
		if (!kv.second.isMap()){
			SHV_EXCEPTION("Invalid chainpack format in config file " + m_usersConfigFileName);
		}

		cp::RpcValue::Map user_params = kv.second.asMap();

		if (kv.first == user_name && password_sha1 == user_params.value("password").toStdString() && !password_sha1.empty()){
			cp::RpcValue grants = user_params.value("grants");
			return (grants.isList()) ? grants.asList() : cp::RpcValue::List();
		}
	}

	return cp::RpcValue("");
}

