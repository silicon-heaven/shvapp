#include "appclioptions.h"

namespace cp = shv::chainpack;

AppCliOptions::AppCliOptions()
{
	serverHost_optionRef().setDefaultValue("nirvana.elektroline.cz");
	serverPort_optionRef().setDefaultValue(3756);
	//user_optionRef().setDefaultValue("shvsitesapp");
	//password_optionRef().setDefaultValue("8884a26b82a69838092fd4fc824bbfde56719e02");
	//loginType_optionRef().setDefaultValue("sha1");
	addOption("app.testMode").setType(cp::RpcValue::Type::Bool).setNames("--tst", "--test-mode").setComment("Run application test mode");
	addOption("device.shvPath").setType(cp::RpcValue::Type::String).setNames("-t", "--device-shv-path").setComment("Device SHV path");
	addOption("device.logFile").setType(cp::RpcValue::Type::String).setNames("-f", "--log-file").setComment("Device log file");
}
