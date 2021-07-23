#pragma once

#include <shv/iotqt/rpc/deviceappclioptions.h>

class AppCliOptions : public shv::iotqt::rpc::DeviceAppCliOptions
{
	using Super = shv::iotqt::rpc::DeviceAppCliOptions;

public:
	AppCliOptions();

	CLIOPTION_GETTER_SETTER2(std::string, "path", p, setP, ath)
	CLIOPTION_GETTER_SETTER2(std::string, "method", m, setM, ethod)
	CLIOPTION_GETTER_SETTER2(std::string, "params", p, setP, arams)
};
