#include "appclioptions.h"

AppCliOptions::AppCliOptions()
{
	addOption("version").setType(shv::chainpack::RpcValue::Type::Bool).setNames("--version").setComment("Application version");
	addOption("app.dataDir").setType(shv::chainpack::RpcValue::Type::String).setNames("--datadir")
			.setComment("Local file system directory, which contains log files.");
	addOption("shvSitesPath").setType(shv::chainpack::RpcValue::Type::String).setNames("--shvSitesPath")
			.setComment("Maintained path in shvtree");
	addOption("elesysPath").setType(shv::chainpack::RpcValue::Type::String).setNames("--elesysPath")
			.setComment("path to elesys node").setDefaultValue("elesys");
	addOption("sitesPath").setType(shv::chainpack::RpcValue::Type::String).setNames("--sitesPath")
			.setComment("path to sites node").setDefaultValue("sites");
	addOption("app.test").setType(shv::chainpack::RpcValue::Type::Bool).setNames("--test")
			.setComment("runs in test mode, shv/test/.. asks from elesys as \"normal\", otherwise doesn't care about them")
			.setDefaultValue(false);
}
