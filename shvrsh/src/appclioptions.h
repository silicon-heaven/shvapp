#pragma once

#include <shv/iotqt/rpc/clientappclioptions.h>

class AppCliOptions : public shv::iotqt::rpc::ClientAppCliOptions
{
private:
	using Super = shv::iotqt::rpc::ClientAppCliOptions;
public:
	AppCliOptions();

	CLIOPTION_GETTER_SETTER2(std::string, "tunnel.shvPath", t, setT, unnelShvPath)
	CLIOPTION_GETTER_SETTER2(std::string, "tunnel.method", t, setT, unnelMethod)
	//CLIOPTION_GETTER_SETTER2(QString, "rexec.command", e, setE, xecCommand)
};

