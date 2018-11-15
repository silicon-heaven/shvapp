#pragma once

#include <shv/core/utils/clioptions.h>

#include <QSet>

class AppCliOptions : public shv::core::utils::ConfigCLIOptions
{
private:
	using Super = shv::core::utils::ConfigCLIOptions;

	CLIOPTION_GETTER_SETTER2(bool, "rpc.metaTypeExplicit", is, set, MetaTypeExplicit)
public:
	AppCliOptions();
	//~AppCliOptions() Q_DECL_OVERRIDE {}
};

