#include "appclioptions.h"

AppCliOptions::AppCliOptions()
	: Super()
{
	addOption("fs.rootDir").setType(shv::chainpack::RpcValue::Type::String).setNames("--fs", "--fs-rootdir")
			.setComment("Local file system directory, which will be exported.");
}
