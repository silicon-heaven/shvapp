#include "appclioptions.h"

namespace cp = shv::chainpack;

AppCliOptions::AppCliOptions()
{
	addOption("telegram.enabled").setType(cp::RpcValue::Type::Bool)
			.setNames("--tg", "--telegram-enabled");
			//.setDefaultValue("894962268:AAHoTaHnlNWDfe3xJWN_RHjn6rxvYQSaGQI");
}
