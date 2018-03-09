#include "appclioptions.h"

AppCliOptions::AppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("locale").setType(QVariant::String).setNames("--locale").setComment(tr("Application locale")).setDefaultValue("system");
	addOption("profile").setType(QVariant::String).setNames("--profile").setComment("Server profile name").setDefaultValue("eyassrv");
	addOption("server.port").setType(QVariant::Int).setNames("--server-port").setComment("Server port").setDefaultValue(3755);
	addOption("sql.host").setType(QVariant::String).setNames("-s", "--sql-host").setComment("SQL server host");
	addOption("sql.port").setType(QVariant::Int).setNames("--sql-port").setComment("SQL server port").setDefaultValue(5432);
	addOption("debug.echoEnabled").setType(QVariant::Bool).setNames("--echo-enabled").setComment("Enable 'echo' debug request before login").setDefaultValue(false);
	addOption("rpc.metaTypeImplicit").setType(QVariant::Bool).setNames("--rpc-metatype-implicit").setComment(tr("Type ID is not included in RpcMessage when set")).setDefaultValue(false);
}
