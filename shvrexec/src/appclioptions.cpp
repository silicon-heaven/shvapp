#include "appclioptions.h"

AppCliOptions::AppCliOptions()
{
	/*
	addOption("rexec.command").setType(QVariant::String).setNames("-e", "--command")
			.setComment(tr("Command to execute on remote node."));
	addOption("rexec.winSize").setType(QVariant::String).setNames("-w", "--win-size")
			.setComment(tr("Terminal window size in form WIDTHxHEIGHT ie: 80x25 ."));
	*/

	setHeartBeatInterval(0);
}
