#include "shvagentapp.h"
#include "appclioptions.h"

#include <shv/chainpack/rpcmessage.h>

#include <shv/core/utils.h>

#include <shv/coreqt/log.h>
#include <shv/coreqt/utils.h>

#include <QTextStream>
#include <QTranslator>
#include <QDateTime>

#include <iostream>

int main(int argc, char *argv[])
{
	// call something from shv::coreqt to avoid linker error:
	// error while loading shared libraries: libshvcoreqt.so.1: cannot open shared object file: No such file or directory
	shv::coreqt::Utils::qVariantToRpcValue(QVariant());

	QCoreApplication::setOrganizationName("Elektroline");
	QCoreApplication::setOrganizationDomain("elektroline.cz");
	QCoreApplication::setApplicationName("shvagent");
	QCoreApplication::setApplicationVersion("0.0.2");

	std::vector<std::string> shv_args = NecroLog::setCLIOptions(argc, argv);

	NecroLog::registerTopic("Tester", "Tester");
	NecroLog::registerTopic("RunCmd", "Run shell commands, scripts");

	int ret = 0;

	AppCliOptions cli_opts;
	cli_opts.parse(shv_args);
	if(cli_opts.isParseError()) {
		for(const std::string &err : cli_opts.parseErrors())
			shvError() << err;
		return EXIT_FAILURE;
	}
	if(cli_opts.isAppBreak()) {
		if(cli_opts.isHelp()) {
			cli_opts.printHelp(std::cout);
		}
		return EXIT_SUCCESS;
	}
	for(const std::string &s : cli_opts.unusedArguments()) {
		shvWarning() << "Undefined argument:" << s;
	}

	if(!cli_opts.loadConfigFile()) {
		return EXIT_FAILURE;
	}

	shv::chainpack::RpcMessage::registerMetaTypes();

	shvInfo() << "======================================================================================";
	shvInfo() << "Starting SHV AGENT server, PID:" << QCoreApplication::applicationPid() << "build:" << __DATE__ << __TIME__;
#ifdef GIT_COMMIT
	shvInfo() << "GIT commit:" << SHV_EXPAND_AND_QUOTE(GIT_COMMIT);
#endif
	shvInfo() << QDateTime::currentDateTime().toString(Qt::ISODate).toStdString() << "UTC:" << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
	shvInfo() << "======================================================================================";
	shvInfo() << "Log tresholds:" << NecroLog::thresholdsLogInfo();
	if(cli_opts.connStatusUpdateInterval() > 0)
		shvInfo() << "Connection status file:" << cli_opts.connStatusFile();
	shvInfo() << "--------------------------------------------------------------------------------------";

	ShvAgentApp a(argc, argv, &cli_opts);

	shvInfo() << "starting main thread event loop";
	ret = a.exec();
	shvInfo() << "main event loop exit code:" << ret;
	shvInfo() << "bye ...";

	return ret;
}

