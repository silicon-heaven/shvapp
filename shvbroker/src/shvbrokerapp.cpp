#include "shvbrokerapp.h"

#include <shv/coreqt/log.h>

ShvBrokerApp::ShvBrokerApp(int &argc, char **argv, AppCliOptions *cli_opts)
	: Super(argc, argv, cli_opts)
{

}

ShvBrokerApp::~ShvBrokerApp()
{
	shvInfo() << "Destroying SHV BROKER application object";
}

QString ShvBrokerApp::versionString() const
{
	return QCoreApplication::applicationVersion();
}

AppCliOptions *ShvBrokerApp::cliOptions()
{
	return dynamic_cast<AppCliOptions*>(Super::cliOptions());
}

