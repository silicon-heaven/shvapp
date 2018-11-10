#include "appclioptions.h"

AppCliOptions::AppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("locale").setType(QVariant::String).setNames("--locale").setComment(tr("Application locale")).setDefaultValue("system");
	//addOption("profile").setType(QVariant::String).setNames("--profile").setComment("Server profile name").setDefaultValue("eyassrv");
	addOption("server.port").setType(QVariant::Int).setNames("--server-port").setComment("Server port").setDefaultValue(3755);
	addOption("sql.host").setType(QVariant::String).setNames("-s", "--sql-host").setComment("SQL server host");
	addOption("sql.port").setType(QVariant::Int).setNames("--sql-port").setComment("SQL server port").setDefaultValue(5432);
	addOption("rpc.metaTypeExplicit").setType(QVariant::Bool).setNames("--mtid", "--rpc-metatype-explicit").setComment(tr("RpcMessage Type ID is included in RpcMessage when set, for more verbose -v rpcmsg log output")).setDefaultValue(false);

	addOption("etc.acl.fstab").setType(QVariant::String).setNames("--fstab")
			.setComment("File with deviceID->mountPoint mapping, if it is relative path, {config-dir} is prepended.")
			.setDefaultValue("fstab.cpon");
	addOption("etc.acl.users").setType(QVariant::String).setNames("--users")
			.setComment("File with shv users definition, if it is relative path, {config-dir} is prepended.")
			.setDefaultValue("users.cpon");
	addOption("etc.acl.grants").setType(QVariant::String).setNames("--grants")
			.setComment("File with grants definition, if it is relative path, {config-dir} is prepended.")
			.setDefaultValue("grants.cpon");
	addOption("etc.acl.paths").setType(QVariant::String).setNames("--paths")
			.setComment("File with shv node paths access rights definition, if it is relative path, {config-dir} is prepended.")
			.setDefaultValue("paths.cpon");
	addOption("slaveConnections").setType(QVariant::Map).setComment("Can be used from config file only.");
}
