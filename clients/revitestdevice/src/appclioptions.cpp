#include "appclioptions.h"

AppCliOptions::AppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("app.simBattVoltageDrift").setType(QVariant::Bool).setNames("--bd", "--battery-voltage-drift")
			.setComment(tr("Simulate battery voltage drift")).setDefaultValue(true);
}
