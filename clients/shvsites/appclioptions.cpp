#include "appclioptions.h"

AppCliOptions::AppCliOptions()
{
	serverHost_optionRef().setDefaultValue("nirvana.elektroline.cz");
	serverPort_optionRef().setDefaultValue(3756);
	user_optionRef().setDefaultValue("shvsitesapp");
	password_optionRef().setDefaultValue("8884a26b82a69838092fd4fc824bbfde56719e02");
	loginType_optionRef().setDefaultValue("sha1");
}
