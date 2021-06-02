#include "appclioptions.h"

AppCliOptions::AppCliOptions()
{
	addOption("app.localSitesDir").setType(shv::chainpack::RpcValue::Type::String).setNames("--lsd", "--local-sites-dir")
			.setComment("Local file system directory, which contains shv config files.")
			.setDefaultValue("/tmp/sites");
	addOption("app.remoteSitesUrl").setType(shv::chainpack::RpcValue::Type::String).setNames("--rsu", "--remote-sites-url")
			.setComment("Url location to remote server file which contains all Elektroline sites.")
			.setDefaultValue("https://gitlab.com/revitest-predator/sites/raw/master/sites.json");
}

std::string AppCliOptions::sitesFilePath()
{
	return effectiveConfigDir() + "/sites.json";
}
