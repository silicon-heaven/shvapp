#ifndef BRCLABUSERS_H
#define BRCLABUSERS_H

#include <QObject>

#include <shv/chainpack/rpcvalue.h>

class BrclabUsers: public QObject
{
	Q_OBJECT
public:
	BrclabUsers(const std::string &config_file_name, QObject *parent);
	bool addUser(const std::string &user_name, const std::string &password_sha1, const shv::chainpack::RpcValue &grants);
	bool changePassword(const std::string &user_name, const std::string &new_password_sha1);
	shv::chainpack::RpcValue getUserGrants(const std::string &user_name, const std::string &password_sha1);

private:
	shv::chainpack::RpcValue loadUsersConfig();
	void reloadUsersConfig();
	void saveUsersConfig(const shv::chainpack::RpcValue &data);

	const shv::chainpack::RpcValue &usersConfig();
	std::string m_usersConfigFileName;
	shv::chainpack::RpcValue m_usersConfig;
};

#endif // BRCLABUSERS_H
