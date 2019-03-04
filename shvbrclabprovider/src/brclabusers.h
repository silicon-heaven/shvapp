#ifndef BRCLABUSERS_H
#define BRCLABUSERS_H

#include <QObject>

#include <shv/chainpack/rpcvalue.h>

class BrclabUsers: public QObject
{
	Q_OBJECT
public:
	BrclabUsers(const std::string &config_file_name, QObject *parent);
	bool addUser(const shv::chainpack::RpcValue &params);
	bool changePassword(const shv::chainpack::RpcValue &params);
	shv::chainpack::RpcValue getUserGrants(const shv::chainpack::RpcValue &params);
	shv::chainpack::RpcValue loadUsersConfig();

private:
	void reloadUsersConfig();
	void setUsersConfig(const shv::chainpack::RpcValue &data);

	const shv::chainpack::RpcValue &usersConfig();
	std::string m_usersConfigFileName;
	shv::chainpack::RpcValue m_usersConfig;
};

#endif // BRCLABUSERS_H
