#include "appclioptions.h"

namespace cp = shv::chainpack;

AppCliOptions::AppCliOptions()
{
	addOption("app.luaFile").setType(cp::RpcValue::Type::String).setNames("--lua-file")
			.setComment("Lua file to run")
			.setMandatory(true);
}
