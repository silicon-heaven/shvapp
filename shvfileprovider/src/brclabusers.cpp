#include "brclabusers.h"
#include "appclioptions.h"

#include "shv/core/log.h"
#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/cponreader.h>

#include <fstream>

namespace cp = shv::chainpack;

BrclabUsers::BrclabUsers(const std::string &users_config_fn, QObject *parent):
	QObject(parent)
{
	m_usersConfigFileName = users_config_fn;
}

shv::chainpack::RpcValue BrclabUsers::loadUsersConfig()
{
	shv::chainpack::RpcValue ret;

	try{
		std::ifstream ifs(m_usersConfigFileName);

		if (!ifs.good()) {
			throw std::runtime_error("Input stream is broken.");
		}

		cp::CponReader rd(ifs);
		std::string err;
		rd.read(ret, err);

		if (!err.empty()) {
			throw std::runtime_error(err);
		}
	}
	catch (std::exception &e) {
		shvError() << "Cannot open file" << m_usersConfigFileName << e.what();
		return cp::RpcValue::Map();
	}

	return ret;
}

void BrclabUsers::saveUsersConfig(const cp::RpcValue &data)
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
		shvInfo() << "Users were updated in" << m_usersConfigFileName;
	}
	else{
		SHV_EXCEPTION("Config must be RpcValue::Map type, config name: " + m_usersConfigFileName);
	}
}

const shv::chainpack::RpcValue &BrclabUsers::usersConfig()
{
	if (!m_usersConfig.isMap()){
		m_usersConfig = loadUsersConfig();
	}

	return m_usersConfig;
}

bool BrclabUsers::addUser(const std::string &user_name, const std::string &password_sha1)
{
	cp::RpcValue users_config = loadUsersConfig();

	if (users_config.toMap().hasKey(user_name)){
		SHV_EXCEPTION("User " + user_name + " is already added in config file");
	}

	cp::RpcValue::Map new_users_config = users_config.toMap();
	cp::RpcValue::Map user_config;

	user_config["grants"] = cp::RpcValue::List();
	user_config["password"] = password_sha1;
	new_users_config[user_name] = user_config;

	saveUsersConfig(new_users_config);
	reloadUsersConfig();

	return true;
}

bool BrclabUsers::changePassword(const std::string &user_name, const std::string &new_password_sha1)
{
	cp::RpcValue::Map users = loadUsersConfig().toMap();

	if (users.hasKey(user_name)){
		cp::RpcValue::Map user = users.at(user_name).toMap();
		user["password"] = new_password_sha1;
		users.at(user_name) = user;

		saveUsersConfig(users);
		reloadUsersConfig();
	}
	else{
		SHV_EXCEPTION("User " + user_name + " does not exist.");
	}

	return true;
}

shv::chainpack::RpcValue BrclabUsers::getUserGrants(const std::string &user_name, const std::string &password_sha1)
{
	cp::RpcValue users_config = usersConfig();

	if (!users_config.isMap()){
		SHV_EXCEPTION("Invalid chainpack format in config file " + m_usersConfigFileName);
	}

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

void BrclabUsers::reloadUsersConfig()
{
	m_usersConfig = cp::RpcValue();
	loadUsersConfig();
}

