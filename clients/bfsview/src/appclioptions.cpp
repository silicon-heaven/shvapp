#include "appclioptions.h"

namespace cp = shv::chainpack;

AppCliOptions::AppCliOptions()
{
	addOption("pwrStatus.publishInterval").setType(cp::RpcValue::Type::Int).setNames("-i", "--pwr-status-publish-interval").setDefaultValue(5)
			.setComment("How often the pwrStatus value will be sent as notification [sec], disabled when == 0.");
	addOption("pwrStatus.simulate").setType(cp::RpcValue::Type::Bool).setNames("--pwr-status-simulate").setDefaultValue(false);
}
