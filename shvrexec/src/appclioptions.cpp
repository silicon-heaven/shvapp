#include "appclioptions.h"

AppCliOptions::AppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("rexec.command").setType(QVariant::String).setNames("-e", "--command")
			.setComment(tr("Command to execute on remote node."));

	setHeartbeatInterval(0);
}
