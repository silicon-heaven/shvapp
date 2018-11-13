#include "appclioptions.h"

AppCliOptions::AppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("sysfs.rootDir").setType(QVariant::String).setNames("--fs", "--sysfs-rootdir")
			.setComment(tr("Local file system directory, which will be accessible at sys/fs node."));
}
