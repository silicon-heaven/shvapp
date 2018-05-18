#pragma once

#include <shv/iotqt/rpc/tunnelconnection.h>

class AppCliOptions : public shv::iotqt::rpc::TunnelAppCliOptions
{
	Q_OBJECT
private:
	using Super = shv::iotqt::rpc::TunnelAppCliOptions;
public:
	AppCliOptions(QObject *parent = NULL);
	~AppCliOptions() Q_DECL_OVERRIDE {}

	CLIOPTION_GETTER_SETTER2(QString, "tunnel.shvPath", t, setT, unnelShvPath)
	CLIOPTION_GETTER_SETTER2(QString, "tunnel.method", t, setT, unnelMethod)
	//CLIOPTION_GETTER_SETTER2(QString, "rexec.command", e, setE, xecCommand)
};

