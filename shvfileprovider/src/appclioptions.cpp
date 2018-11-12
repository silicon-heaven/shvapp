#include "appclioptions.h"

AppCliOptions::AppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("sysfs.rootDir").setType(QVariant::String).setNames("--fs", "--sysfs-rootdir")
			.setComment(tr("Local file system directory, which will be accessible at sys/fs node."));
	addOption("app.connStatusFile").setType(QVariant::String).setNames("--cf", "--conn-status-file")
			.setComment(tr("ShvAgent write every 'connStatusUpdateInterval' 1/0 if it is connected/disconnected to shv broker."))
			.setDefaultValue("/tmp/shvagent/conn-status.txt");
	addOption("app.connStatusUpdateInterval").setType(QVariant::Int).setNames("--cu", "--conn-status-update-interval")
			.setComment(tr("Interval to update 'connStatusFile' in sec."))
			.setDefaultValue(0);
}
