#include "appclioptions.h"

namespace cp = shv::chainpack;

AppCliOptions::AppCliOptions()
{
	addOption("tester.enabled").setType(cp::RpcValue::Type::Bool).setNames("--tester")
			.setComment("Enable tester support, which will be accessible at tester node.");
	addOption("tester.script").setType(cp::RpcValue::Type::String).setNames("--ts", "--tester-script")
			.setComment("Enable tester support and start to interpret tester script.");

	addOption("sysfs.rootDir").setType(cp::RpcValue::Type::String).setNames("--fs", "--sysfs-rootdir")
			.setComment("Local file system directory, which will be accessible at sys/fs node.");
	addOption("app.connStatusFile").setType(cp::RpcValue::Type::String).setNames("--cf", "--conn-status-file")
			.setComment("ShvAgent write every 'connStatusUpdateInterval' 1/0 if it is connected/disconnected to shv broker.")
			.setDefaultValue("/tmp/shvagent/conn-status.txt");
	addOption("app.connStatusUpdateInterval").setType(cp::RpcValue::Type::Int).setNames("--cu", "--conn-status-update-interval")
			.setComment("Interval to update 'connStatusFile' in sec.")
			.setDefaultValue(0);
}
