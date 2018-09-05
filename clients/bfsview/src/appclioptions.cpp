#include "appclioptions.h"

AppCliOptions::AppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("pwrStatus.publishInterval").setType(QVariant::Int).setNames("-i", "--pwr-status-publish-interval").setDefaultValue(5)
			.setComment(tr("How often the pwrStatus value will be sent as notification [sec], disabled when == 0."));
	addOption("pwrStatus.simulate").setType(QVariant::Bool).setNames("--pwr-status-simulate").setDefaultValue(false);
}
