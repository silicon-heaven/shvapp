#include "appclioptions.h"

namespace cp = shv::chainpack;

AppCliOptions::AppCliOptions()
{
	addOption("app.simBattVoltageDrift").setType(cp::RpcValue::Type::Bool).setNames("--bd", "--battery-voltage-drift")
			.setComment("Simulate battery voltage drift").setDefaultValue(true);
}
