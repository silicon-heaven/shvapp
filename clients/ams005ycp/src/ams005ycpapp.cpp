#include "ams005ycpapp.h"
#include "appclioptions.h"
#include "settings.h"

#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/iotqt/node/localfsnode.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/metamethod.h>

#include <shv/core/stringview.h>
#include <shv/core/assert.h>

#include <QFileSystemWatcher>
#include <QSettings>
#include <QTimer>

#include <fstream>

namespace cp = shv::chainpack;

Ams005YcpApp::Ams005YcpApp(int &argc, char **argv, AppCliOptions* cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	loadSettings();
}

Ams005YcpApp::~Ams005YcpApp()
{
	shvInfo() << "destroying shv agent application";
}

Ams005YcpApp *Ams005YcpApp::instance()
{
	return qobject_cast<Ams005YcpApp *>(QCoreApplication::instance());
}

void Ams005YcpApp::loadSettings()
{
	/*
	Settings settings;
	AppCliOptions *cli_opts = cliOptions();
	if(!cli_opts->serverHost_isset())
		cli_opts->setServerHost(settings.shvBrokerHost().toStdString());
	if(!cli_opts->user_isset())
		cli_opts->setUser("jn50view");
	if(!cli_opts->serverPort_isset())
		cli_opts->setServerPort(settings.shvBrokerPort());
	if(!cli_opts->password_isset()) {
		cli_opts->setPassword("8884a26b82a69838092fd4fc824bbfde56719e02");
		cli_opts->setLoginType("SHA1");
	}
	*/
}

QVariant Ams005YcpApp::reloadOpcValue(const std::string &path)
{
#warning Ams005YcpApp::reloadOpcValue NIY
	return QVariant();
}

QVariant Ams005YcpApp::opcValue(const std::string &path)
{
#warning Ams005YcpApp::opcValue NIY
	return QVariant();
}
/*
const std::string &Ams005YcpApp::logFilePath()
{
	static std::string log_file_path = QDir::tempPath().toStdString() + "/" + applicationName().toStdString() + ".log";
	return log_file_path;
}
*/
