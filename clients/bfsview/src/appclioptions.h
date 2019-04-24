#pragma once

#include <shv/iotqt/rpc/deviceappclioptions.h>

class AppCliOptions : public shv::iotqt::rpc::DeviceAppCliOptions
{
	using Super = shv::iotqt::rpc::DeviceAppCliOptions;
public:
	AppCliOptions();

	CLIOPTION_GETTER_SETTER2(int, "pwrStatus.publishInterval", p, setP, wrStatusPublishInterval)
	CLIOPTION_GETTER_SETTER2(bool, "pwrStatus.simulate", is, set, PwrStatusSimulate)
};

