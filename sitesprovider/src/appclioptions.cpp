#include "appclioptions.h"

AppCliOptions::AppCliOptions()
{
	addOption("app.dataDir").setType(shv::chainpack::RpcValue::Type::String).setNames("--datadir")
			.setComment("Local file system directory, which contains shv config files.");
	addOption("app.remoteSitesUrl").setType(shv::chainpack::RpcValue::Type::String).setNames("--rsu", "--remote--sites-url")
			.setComment("Url location to remote server file which contains all Elektroline sites.")
			.setDefaultValue("https://raw.githubusercontent.com/silicon-heaven/sites/master/sites.json");
}
