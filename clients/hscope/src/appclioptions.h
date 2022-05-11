#pragma once

#include <shv/iotqt/rpc/deviceappclioptions.h>

class AppCliOptions : public shv::iotqt::rpc::DeviceAppCliOptions
{
private:
	using Super = shv::iotqt::rpc::DeviceAppCliOptions;
public:
	AppCliOptions();
};

