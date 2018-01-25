#include "theapp.h"
#include "appclioptions.h"

#include <shv/core/log.h>

TheApp::TheApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv, cli_opts)
{

}

TheApp::~TheApp()
{
	shvInfo() << "destroying shv agent application";
}
