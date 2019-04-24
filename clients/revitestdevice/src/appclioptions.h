#pragma once

#include <shv/iotqt/rpc/deviceappclioptions.h>

class AppCliOptions : public shv::iotqt::rpc::DeviceAppCliOptions
{
private:
	using Super = shv::iotqt::rpc::DeviceAppCliOptions;
public:
	AppCliOptions();

	CLIOPTION_GETTER_SETTER2(bool, "app.simBattVoltageDrift", is, set, SimBattVoltageDrift)
	CLIOPTION_GETTER_SETTER2(int, "app.deviceCount", d, setD, eviceCount)
	CLIOPTION_GETTER_SETTER2(std::string, "app.callMethods", c, setC, allMethods)
	CLIOPTION_GETTER_SETTER2(std::string, "app.callFile", c, setC, allFile)
};

