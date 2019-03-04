#include "brclabusers.h"
#include "appclioptions.h"

#include <shv/core/log.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/cponreader.h>

#include <QFile>
#include <fstream>

namespace cp = shv::chainpack;

BrclabUsers::BrclabUsers(const std::string &config_file_name, QObject *parent):
	QObject(parent)
{
	m_usersConfigFileName = config_file_name;
	m_usersConfig = loadUsersConfig();
}

shv::chainpack::RpcValue BrclabUsers::loadUsersConfig()
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

void BrclabUsers::setUsersConfig(const shv::chainpack::RpcValue &data)
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

const shv::chainpack::RpcValue &BrclabUsers::usersConfig()
{
	if (!m_usersConfig.isMap()){
		m_usersConfig = loadUsersConfig();
	}

	return m_usersConfig;
}

bool BrclabUsers::addUser(const cp::RpcValue &params)
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

bool BrclabUsers::changePassword(const cp::RpcValue &params)
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

shv::chainpack::RpcValue BrclabUsers::getUserGrants(const cp::RpcValue &params)
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
