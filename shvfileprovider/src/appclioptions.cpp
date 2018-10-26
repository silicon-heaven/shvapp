#include "appclioptions.h"

AppCliOptions::AppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("sysfs.rootDir").setType(QVariant::String).setNames("--fs", "--sysfs-rootdir")
			.setComment(tr("Local file system directory, which will be accessible at sys/fs node."));
	addOption("app.connStatusFile").setType(QVariant::String).setNames("--cf", "--conn-status-file")
			.setComment(tr("ShvAgent write every 'connStatusUpdateInterval' 1/0 if it is connected/disconnected to shv broker."))
			.setDefaultValue("/tmp/shvagent/conn-status.txt");
	addOption("app.remoteSitesUrl").setType(QVariant::String).setNames("--rsu", "--remote--sites-url")
			.setComment(tr("Url location to remote server file which contains all Elektroline sites."))
			.setDefaultValue("https://raw.githubusercontent.com/silicon-heaven/sites/master/sites.json");
	addOption("app.localSitesFile").setType(QVariant::String).setNames("--lsf", "--local--sites-file")
			.setComment(tr("Local copy of file which contains all Elektroline sites."))
			.setDefaultValue("/tmp/sites/sites.json");
	addOption("app.connStatusUpdateInterval").setType(QVariant::Int).setNames("--cu", "--conn-status-update-interval")
			.setComment(tr("Interval to update 'connStatusFile' in sec."))
			.setDefaultValue(0);
}
