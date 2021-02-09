#include "appclioptions.h"

#include <shv/chainpack/rpc.h>

namespace cp = shv::chainpack;

AppCliOptions::AppCliOptions()
{
	addOption("tunnel.shvPath").setType(cp::RpcValue::Type::String).setNames("-t", "--tunnel-shvPath").setComment("Tunnel shv path.");
	addOption("tunnel.method").setType(cp::RpcValue::Type::String).setNames("-m", "--tunnel-method").setComment("Open tunnel method.").setDefaultValue(shv::chainpack::Rpc::METH_LAUNCH_REXEC);

	setHeartBeatInterval(0);
}
