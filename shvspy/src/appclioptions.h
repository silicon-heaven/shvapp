#pragma once

#include <shv/core/utils/clioptions.h>

#include <QSet>

class AppCliOptions : public shv::core::utils::ConfigCLIOptions
{
private:
	using Super = shv::core::utils::ConfigCLIOptions;

	//CLIOPTION_GETTER_SETTER2(std::string, "broker.subscribtions", s, setS, ubscriptions)
public:
	AppCliOptions();
	//~AppCliOptions() Q_DECL_OVERRIDE {}
};

