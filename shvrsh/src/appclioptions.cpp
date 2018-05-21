#include "appclioptions.h"

AppCliOptions::AppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("tunnel.shvPath").setType(QVariant::String).setNames("-t", "--tunnel-shvPath").setComment(tr("Tunnel shv path."));
	addOption("tunnel.method").setType(QVariant::String).setNames("-m", "--tunnel-method").setComment(tr("Open tunnel method.")).setDefaultValue(shv::chainpack::Rpc::METH_LAUNCH_REXEC);

	setHeartbeatInterval(0);
}
