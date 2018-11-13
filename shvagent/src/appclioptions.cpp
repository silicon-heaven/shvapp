#include "appclioptions.h"

namespace cp = shv::chainpack;

AppCliOptions::AppCliOptions()
{
	addOption("sysfs.rootDir").setType(cp::RpcValue::Type::String).setNames("--fs", "--sysfs-rootdir")
			.setComment("Local file system directory, which will be accessible at sys/fs node.");
	addOption("app.connStatusFile").setType(cp::RpcValue::Type::String).setNames("--cf", "--conn-status-file")
			.setComment("ShvAgent write every 'connStatusUpdateInterval' 1/0 if it is connected/disconnected to shv broker.")
			.setDefaultValue("/tmp/shvagent/conn-status.txt");
	addOption("app.connStatusUpdateInterval").setType(cp::RpcValue::Type::Int).setNames("--cu", "--conn-status-update-interval")
			.setComment("Interval to update 'connStatusFile' in sec.")
			.setDefaultValue(0);
}
