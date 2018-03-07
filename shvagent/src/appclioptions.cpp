#include "appclioptions.h"

AppCliOptions::AppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("protocol.type").setType(QVariant::String).setNames("--protocol-type").setComment(tr("Protocol type [chainpack | cpon | jsonrpc]")).setDefaultValue("chainpack");
}
