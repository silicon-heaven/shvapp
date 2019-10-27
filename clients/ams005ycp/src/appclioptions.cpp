#include "appclioptions.h"

namespace cp = shv::chainpack;

AppCliOptions::AppCliOptions()
{
	addOption("zone.name").setType(cp::RpcValue::Type::String)
			.setNames("-z", "--zone-name")
			.setDefaultValue("D");
}
