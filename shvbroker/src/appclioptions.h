#pragma once

#include <shv/broker/appclioptions.h>

class AppCliOptions : public shv::broker::AppCliOptions
{
private:
	using Super = shv::broker::AppCliOptions;
public:
	AppCliOptions();
	~AppCliOptions() override {}
};

