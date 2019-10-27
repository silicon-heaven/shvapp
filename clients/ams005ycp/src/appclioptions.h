#pragma once

#include <shv/core/utils/clioptions.h>

class AppCliOptions : public shv::core::utils::ConfigCLIOptions
{
	using Super = shv::core::utils::ConfigCLIOptions;
public:
	AppCliOptions();

	CLIOPTION_GETTER_SETTER2(std::string, "zone.name", z, setZ, oneName)
};

