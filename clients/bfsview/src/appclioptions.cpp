#include "appclioptions.h"

AppCliOptions::AppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("pwrStatus.publishInterval").setType(QVariant::Int).setNames("-i", "--pwr-status-publish-interval")
			.setComment(tr("How often the pwrStatus value will be sent as notification [sec], disabled when == 0."));
}
