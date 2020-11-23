#include "appclioptions.h"

AppCliOptions::AppCliOptions()
{
	addOption("version").setType(shv::chainpack::RpcValue::Type::Bool).setNames("--version").setComment("Application version");
	addOption("app.logCacheDir").setType(shv::chainpack::RpcValue::Type::String).setNames("--log-cache-dir")
			.setComment("Local file system directory, which contains log files.").setDefaultValue("/tmp/hp");
	addOption("app.sitesRootPath").setType(shv::chainpack::RpcValue::Type::String).setNames("--sites-root-path")
			.setComment("Maintained path in shvtree");
	addOption("app.trimDirtyLogInterval").setType(shv::chainpack::RpcValue::Type::Int).setNames("--trim-dirty-log-interval")
			.setComment("interval of dirty log trimming (mins)").setDefaultValue(30);
	addOption("elesysPath").setType(shv::chainpack::RpcValue::Type::String).setNames("--elesys-path")
			.setComment("path to elesys node").setDefaultValue("elesys");
	addOption("sitesPath").setType(shv::chainpack::RpcValue::Type::String).setNames("--sites-path")
			.setComment("path to sites node").setDefaultValue("sites");
	addOption("app.test").setType(shv::chainpack::RpcValue::Type::Bool).setNames("--test")
			.setComment("runs in test mode, shv/test/.. asks from elesys as \"normal\", otherwise doesn't care about them")
			.setDefaultValue(false);
	addOption("app.cacheSizeLimit").setType(shv::chainpack::RpcValue::Type::String).setNames("--cache-size-limit")
			.setComment("set cache size limit (suffixes: k, M, G)").setDefaultValue("0");
}
