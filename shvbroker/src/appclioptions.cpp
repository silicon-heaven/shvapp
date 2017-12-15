#include "appclioptions.h"

AppCliOptions::AppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("locale").setType(QVariant::String).setNames("--locale").setComment(tr("Application locale")).setDefaultValue("system");
	addOption("profile").setType(QVariant::String).setNames("--profile").setComment("Server profile name").setDefaultValue("eyassrv");
	addOption("server.port").setType(QVariant::Int).setNames("--server-port").setComment("Server port").setDefaultValue(3755);
	addOption("sql.host").setType(QVariant::String).setNames("-s", "--sql-host").setComment("SQL server host");
	addOption("sql.port").setType(QVariant::Int).setNames("--sql-port").setComment("SQL server port").setDefaultValue(5432);
}
