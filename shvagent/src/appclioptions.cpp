#include "appclioptions.h"

AppCliOptions::AppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("locale").setType(QVariant::String).setNames("--locale").setComment(tr("Application locale")).setDefaultValue("system");
	addOption("user.name").setType(QVariant::String).setNames("-u", "--user").setComment(tr("User name"));
	addOption("server.host").setType(QVariant::String).setNames("-s", "--server-host").setComment(tr("server host")).setDefaultValue("localhost");
	addOption("server.port").setType(QVariant::Int).setNames("-p", "--server-port").setComment(tr("server port")).setDefaultValue(3755);
	addOption("rpc.timeout").setType(QVariant::Int).setNames("--rpc-timeout").setComment(tr("RPC timeout msec")).setDefaultValue(5000);
}
