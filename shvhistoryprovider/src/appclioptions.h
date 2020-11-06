#pragma once

#include <shv/iotqt/rpc/deviceappclioptions.h>

class AppCliOptions : public shv::iotqt::rpc::DeviceAppCliOptions
{
	using Super = shv::iotqt::rpc::DeviceAppCliOptions;

public:
	AppCliOptions();

	CLIOPTION_GETTER_SETTER2(bool, "version", is, set, Version)
	CLIOPTION_GETTER_SETTER2(std::string, "app.logCacheDir", l, setL, ogCacheDir)
	CLIOPTION_GETTER_SETTER2(std::string, "app.sitesRootPath", s, setS, itesRootPath)
	CLIOPTION_GETTER_SETTER2(int, "app.trimDirtyLogInterval", t, setT, rimDirtyLogInterval)
	CLIOPTION_GETTER_SETTER2(std::string, "sitesPath", s, setS, itesPath)
	CLIOPTION_GETTER_SETTER2(std::string, "elesysPath", e, setE, lesysPath)
	CLIOPTION_GETTER_SETTER2(bool, "app.test", t, setT, est)
	CLIOPTION_GETTER_SETTER2(std::string, "app.cacheSizeLimit", c, setC, acheSizeLimit)
};

