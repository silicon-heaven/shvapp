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
static const char M_CHANGE_USER_PASSWORD[] = "changeUserPassword";
static const char M_GET_USER_GRANTS[] = "getUserGrants";

static std::vector<cp::MetaMethod> meta_methods {
	{M_ADD_USER, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_WRITE},
	{M_CHANGE_USER_PASSWORD, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_WRITE},
	{M_GET_USER_GRANTS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_WRITE}
};

BrclabUsersNode::BrclabUsersNode(const std::string &node_id, const std::string &fn, ShvNode *parent)
	: Super(node_id, parent)
{
	m_usersConfigFileName = fn;
	m_usersConfig = loadUsersConfig();
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

shv::chainpack::RpcValue BrclabUsersNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == M_ADD_USER) {
			return addUser(params);
		}
		else if(method == M_CHANGE_USER_PASSWORD) {
			return changePassword(params);
		}
		else if(method == M_GET_USER_GRANTS) {
			return getUserGrants(params);
		}
	}

	return Super::callMethod(shv_path, method, params);
}

void BrclabUsersNode::loadValues()
{
	m_values = loadUsersConfig();
}

void BrclabUsersNode::saveValues(void)
{
	setUsersConfig(m_values);
	Super::loadValues();
}

shv::chainpack::RpcValue BrclabUsersNode::loadUsersConfig()
{
	shv::chainpack::RpcValue ret;

	if (!QFile::exists(QString::fromStdString(m_usersConfigFileName))){
		setUsersConfig(cp::RpcValue::Map());
	}

	std::ifstream ifs(m_usersConfigFileName);
	ifs.exceptions( std::ifstream::failbit | std::ifstream::badbit );

	if (!ifs.good()) {
		SHV_EXCEPTION("Cannot open file " + m_usersConfigFileName +  " for reading!");
	}

	cp::CponReader rd(ifs);
	rd.read(ret);

	if (!ret.isMap()){
		SHV_EXCEPTION("Config file " + m_usersConfigFileName + " must be a Map!");
	}

	return ret;
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
		SHV_EXCEPTION("Config file must be a Map, config name: " + m_usersConfigFileName);
	}

	m_usersConfig = loadUsersConfig();
}

const shv::chainpack::RpcValue &BrclabUsersNode::usersConfig()
{
	if (!m_usersConfig.isMap()){
		m_usersConfig = loadUsersConfig();
	}

	return m_usersConfig;
}

bool BrclabUsersNode::addUser(const cp::RpcValue &params)
{
	if (!params.isMap() || !params.toMap().hasKey("user") || !params.toMap().hasKey("password")){
		SHV_EXCEPTION("Invalid parameters format. Params must be a map and it must contains keys: user, password.");
	}

	cp::RpcValue::Map map = params.toMap();
	std::string user_name = map.value("user").toStdString();

	cp::RpcValue::Map users_config = usersConfig().toMap();

	if (users_config.hasKey(user_name)){
		SHV_EXCEPTION("User " + user_name + " already exists");
	}

	cp::RpcValue::Map user;
	const cp::RpcValue grants = map.value("grants");
	user["grants"] = (grants.isList()) ? grants.toList() : cp::RpcValue::List();
	user["password"] = map.value("password").toStdString();
	users_config[user_name] = user;

	setUsersConfig(users_config);
	return true;
}

bool BrclabUsersNode::changePassword(const cp::RpcValue &params)
{
	if (!params.isMap() || !params.toMap().hasKey("user") || !params.toMap().hasKey("newPassword")){
		SHV_EXCEPTION("Invalid parameters format. Params must be a map and it must contains keys: user, newPassword.");
	}

	cp::RpcValue::Map map = params.toMap();
	std::string user_name = map.value("user").toStdString();
	std::string new_password_sha1 = map.value("newPassword").toStdString();

	cp::RpcValue::Map users_config = usersConfig().toMap();

	if (users_config.hasKey(user_name)){
		cp::RpcValue::Map user = users_config.at(user_name).toMap();
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
	if (!params.isMap() || !params.toMap().hasKey("user") || !params.toMap().hasKey("password")){
		SHV_EXCEPTION("Invalid parameters format. Params must be a map and it must contains keys: user, password.");
	}

	cp::RpcValue::Map map = params.toMap();
	std::string user_name = map.value("user").toStdString();
	std::string password_sha1 = map.value("password").toStdString();

	cp::RpcValue users_config = usersConfig();

	for(const auto &kv :users_config.toMap()) {
		if (!kv.second.isMap()){
			SHV_EXCEPTION("Invalid chainpack format in config file " + m_usersConfigFileName);
		}

		cp::RpcValue::Map user_params = kv.second.toMap();

		if (kv.first == user_name && password_sha1 == user_params.value("password").toStdString() && !password_sha1.empty()){
			cp::RpcValue grants = user_params.value("grants");
			return (grants.isList()) ? grants.toList() : cp::RpcValue::List();
		}
	}

	return cp::RpcValue("");
}

