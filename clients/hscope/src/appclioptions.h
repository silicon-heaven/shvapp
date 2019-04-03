#pragma once

#include <shv/iotqt/rpc/deviceappclioptions.h>

class AppCliOptions : public shv::iotqt::rpc::DeviceAppCliOptions
{
	using Super = shv::iotqt::rpc::DeviceAppCliOptions;
public:
	AppCliOptions();

	CLIOPTION_GETTER_SETTER2(bool, "telegram.enabled", isT, setT, elegramEnabled)
};

