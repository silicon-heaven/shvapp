#include "appclioptions.h"

namespace cp = shv::chainpack;

AppCliOptions::AppCliOptions()
{
	addOption("app.journalCacheRoot").setType(shv::chainpack::RpcValue::Type::String).setNames("--journal-cache-root")
		.setComment("Local file system directory, which contains journal cache.").setDefaultValue("/tmp/historyprovider");
	addOption("app.journalCacheSizeLimit").setType(shv::chainpack::RpcValue::Type::String).setNames("--journal-cache-size-limit")
			.setComment("Set journal cache size limit (suffixes: k, M, G)").setDefaultValue("1G");
	addOption("app.journalSanitizerInterval").setType(shv::chainpack::RpcValue::Type::Int).setNames("--journal-sanitizer-interval")
			.setComment("Set journal sanitizer interval").setDefaultValue(60);
}
