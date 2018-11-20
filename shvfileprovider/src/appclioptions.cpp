#include "appclioptions.h"

AppCliOptions::AppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("fs.rootDir").setType(QVariant::String).setNames("--fs", "--fs-rootdir")
			.setComment(tr("Local file system directory, which will be exported."));
}
