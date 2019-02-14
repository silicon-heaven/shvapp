#include "appclioptions.h"

namespace cp = shv::chainpack;

AppCliOptions::AppCliOptions()
{
	addOption("shvconv.shvPath").setType(cp::RpcValue::Type::String).setNames("-t", "--conv-shv-path").setDefaultValue("test/cze/plz/convjn50");
}
