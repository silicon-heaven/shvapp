#include "appclioptions.h"

AppCliOptions::AppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("server.host").setType(QVariant::String).setNames("-s", "--server-host").setComment(tr("SHV Broker host")).setDefaultValue("localhost");
	addOption("server.port").setType(QVariant::Int).setNames("-p", "--server-port").setComment(tr("SHV Broker port")).setDefaultValue(3755);
}
