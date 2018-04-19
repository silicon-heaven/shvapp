#include "appclioptions.h"

AppCliOptions::AppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("protocol.type").setType(QVariant::String).setNames("--protocol-type").setComment(tr("Protocol type [chainpack | cpon | jsonrpc]")).setDefaultValue("chainpack");
	addOption("broker.subscribtions").setType(QVariant::String).setNames("--subscribe").setComment(tr("List of subscriptions on connect in form broker1:path1:method1[,broker2:path2:method2]"));
}
