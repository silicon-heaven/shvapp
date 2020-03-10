#pragma once

#include "appclioptions.h"

#include <shv/iotqt/rpc/password.h>
#include <shv/broker/brokerapp.h>

class EyasSrvCtlApp : public shv::broker::BrokerApp
{
	Q_OBJECT
private:
	using Super = shv::broker::BrokerApp;
public:
	EyasSrvCtlApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~EyasSrvCtlApp() Q_DECL_OVERRIDE;

	QString versionString() const;

	AppCliOptions* cliOptions() override;

	static EyasSrvCtlApp* instance() {return qobject_cast<EyasSrvCtlApp*>(Super::instance());}

	shv::iotqt::rpc::Password password(const std::string &user) override;
protected:
	std::set<std::string> aclUserFlattenRoles(const std::string &user_name) override;
	shv::chainpack::AclRole aclRole(const std::string &role_name) override;
	shv::chainpack::AclRolePaths aclPathsRolePaths(const std::string &role_name) override;
};

