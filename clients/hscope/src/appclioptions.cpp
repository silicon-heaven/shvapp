#include "appclioptions.h"

namespace cp = shv::chainpack;

AppCliOptions::AppCliOptions()
{
	addOption("telegram.enabled").setType(cp::RpcValue::Type::Bool)
			.setNames("--tg", "--telegram-enabled");
	addOption("telegram.logIndented").setType(cp::RpcValue::Type::Bool)
			.setNames("--tgi", "--telegram-log-indented");
}
