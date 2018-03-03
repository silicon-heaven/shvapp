#include "appclioptions.h"

AppCliOptions::AppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("protocol.version").setType(QVariant::String).setNames("--protocol-version").setComment(tr("Protocol version [chainpack | cpon | jsonrpc]")).setDefaultValue("chainpack");
}
