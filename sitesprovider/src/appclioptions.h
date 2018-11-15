#pragma once

#include <shv/iotqt/rpc/deviceconnection.h>

class AppCliOptions : public shv::iotqt::rpc::DeviceAppCliOptions
{
	using Super = shv::iotqt::rpc::DeviceAppCliOptions;

public:
	AppCliOptions();

	CLIOPTION_GETTER_SETTER2(std::string, "app.dataDir", d, setD, ataDir)
	CLIOPTION_GETTER_SETTER2(std::string, "app.remoteSitesUrl", r, setR, emoteSitesUrl)
};

