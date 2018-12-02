#include "appclioptions.h"

namespace cp = shv::chainpack;

AppCliOptions::AppCliOptions()
{
	addOption("locale").setType(cp::RpcValue::Type::String).setNames("--locale").setComment("Application locale").setDefaultValue("system");
	addOption("server.port").setType(cp::RpcValue::Type::Int).setNames("--server-port").setComment("Server port").setDefaultValue(3755);
	addOption("server.publicNode").setType(cp::RpcValue::Type::Bool).setNames("--pbnd", "--server-is-public-node").setComment("Server is public node");
	addOption("sql.host").setType(cp::RpcValue::Type::String).setNames("-s", "--sql-host").setComment("SQL server host");
	addOption("sql.port").setType(cp::RpcValue::Type::Int).setNames("--sql-port").setComment("SQL server port").setDefaultValue(5432);
	addOption("rpc.metaTypeExplicit").setType(cp::RpcValue::Type::Bool).setNames("--mtid", "--rpc-metatype-explicit").setComment("RpcMessage Type ID is included in RpcMessage when set, for more verbose -v rpcmsg log output").setDefaultValue(false);

	addOption("etc.acl.fstab").setType(cp::RpcValue::Type::String).setNames("--fstab")
			.setComment("File with deviceID->mountPoint mapping, if it is relative path, {config-dir} is prepended.")
			.setDefaultValue("fstab.cpon");
	addOption("etc.acl.users").setType(cp::RpcValue::Type::String).setNames("--users")
			.setComment("File with shv users definition, if it is relative path, {config-dir} is prepended.")
			.setDefaultValue("users.cpon");
	addOption("etc.acl.grants").setType(cp::RpcValue::Type::String).setNames("--grants")
			.setComment("File with grants definition, if it is relative path, {config-dir} is prepended.")
			.setDefaultValue("grants.cpon");
	addOption("etc.acl.paths").setType(cp::RpcValue::Type::String).setNames("--paths")
			.setComment("File with shv node paths access rights definition, if it is relative path, {config-dir} is prepended.")
			.setDefaultValue("paths.cpon");
	addOption("masters.connections").setType(cp::RpcValue::Type::Map).setComment("Can be used from config file only.");
	addOption("masters.enabled").setType(cp::RpcValue::Type::Bool).setNames("--mce", "--master-connections-enabled").setComment("Enable slave connections to master broker.");
}
