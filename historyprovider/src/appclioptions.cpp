#include "appclioptions.h"

namespace cp = shv::chainpack;

AppCliOptions::AppCliOptions()
{
	addOption("app.journalCacheRoot").setType(shv::chainpack::RpcValue::Type::String).setNames("--journal-cache-root")
		.setComment("Local file system directory, which contains journal cache.").setDefaultValue("/tmp/historyprovider");
	addOption("app.journalCacheSizeLimit").setType(shv::chainpack::RpcValue::Type::String).setNames("--journal-cache-size-limit")
			.setComment("Set journal cache size limit (suffixes: k, M, G)").setDefaultValue("1G");
	addOption("app.journalSanitizerInterval").setType(shv::chainpack::RpcValue::Type::Int).setNames("--journal-sanitizer-interval")
			.setComment("Set journal sanitizer interval in seconds").setDefaultValue(60);
	addOption("app.logMaxAge").setType(shv::chainpack::RpcValue::Type::Int).setNames("--log-max-age")
			.setComment("Set log max age in seconds: dirty log age or timer-based syncing for (master) pushlog devices").setDefaultValue(60 /*seconds*/ * 60 /*minutes*/ * 1 /*hours*/); // What's a correct value? An hour?
	addOption("app.cacheInitMaxAge").setType(shv::chainpack::RpcValue::Type::Int).setNames("--cache-init-max-age")
			.setComment("Set max age in seconds for initial cache sync").setDefaultValue(60 /*seconds*/ * 60 /*minutes*/ * 24 /*hours*/ * 30 /*days*/); // What's a correct value? An hour?
}
