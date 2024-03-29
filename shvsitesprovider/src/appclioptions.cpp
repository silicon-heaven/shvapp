#include "appclioptions.h"

AppCliOptions::AppCliOptions()
{
	addOption("app.localSitesDir").setType(shv::chainpack::RpcValue::Type::String).setNames("--lsd", "--local-sites-dir")
			.setComment("Local file system directory, which contains shv config files.")
			.setDefaultValue("/tmp/sites-local");
	addOption("app.remoteSitesUrl").setType(shv::chainpack::RpcValue::Type::String).setNames("--rsu", "--remote-sites-url")
			.setComment("Url location to remote server file which contains all Elektroline sites.")
			.setDefaultValue("https://gitlab+deploy-token-478855:uQMhvdiGpd-6FmUAKnQU@gitlab.com/revitest-predator/sites");
	addOption("app.syncSites").setType(shv::chainpack::RpcValue::Type::Bool).setNames("--sync-sites")
			.setComment("Sync sites files with master git repository")
			.setDefaultValue(true);
}
