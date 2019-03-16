#include "appclioptions.h"

namespace cp = shv::chainpack;

AppCliOptions::AppCliOptions()
{
	addOption("app.simBattVoltageDrift").setType(cp::RpcValue::Type::Bool).setNames("--bd", "--battery-voltage-drift")
			.setComment("Simulate battery voltage drift").setDefaultValue(false);
	addOption("app.deviceCount").setType(cp::RpcValue::Type::Int).setNames("-n", "--device-count")
			.setComment("Number of created devices").setDefaultValue(27);
	addOption("app.callMethods").setType(cp::RpcValue::Type::String).setNames("-c", "--call-methods")
			.setComment("List SHV of methods to call after successfull connection to broker.");
}
