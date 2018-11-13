#pragma once

#include <shv/iotqt/rpc/deviceconnection.h>

class AppCliOptions : public shv::iotqt::rpc::DeviceAppCliOptions
{
private:
	using Super = shv::iotqt::rpc::DeviceAppCliOptions;
public:
	AppCliOptions();

	CLIOPTION_GETTER_SETTER2(std::string, "sysfs.rootDir", s, setS, ysFsRootDir)
	CLIOPTION_GETTER_SETTER2(std::string, "app.connStatusFile", c, setC, onnStatusFile)
	CLIOPTION_GETTER_SETTER2(int, "app.connStatusUpdateInterval", c, setC, onnStatusUpdateInterval)
};

