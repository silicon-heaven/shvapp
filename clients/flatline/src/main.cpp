#include "flatlineapp.h"
#include "appclioptions.h"
#include "mainwindow.h"
#include "version.h"

#include <shv/core/utils.h>
#include <shv/coreqt/log.h>
#include <shv/core/utils/shvfilejournal.h>

#include <QTextStream>
#include <QTranslator>
#include <QDateTime>

#include <iostream>

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("Elektroline");
	QCoreApplication::setOrganizationDomain("elektroline.cz");
	QCoreApplication::setApplicationName("flatline");
	QCoreApplication::setApplicationVersion(APP_VERSION);

	std::vector<std::string> shv_args = NecroLog::setCLIOptions(argc, argv);

	shvDebug() << "debug log test";
	shvInfo() << "info log test";
	shvWarning() << "warning log test";
	shvError() << "error log test";

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

	shvInfo() << "======================================================================================";
	shvInfo() << "Starting AncaView, PID:" << QCoreApplication::applicationPid() << "build:" << __DATE__ << __TIME__;
#ifdef GIT_COMMIT
	shvInfo() << "GIT commit:" << SHV_EXPAND_AND_QUOTE(GIT_COMMIT);
#endif
	shvInfo() << QDateTime::currentDateTime().toString(Qt::ISODate).toStdString() << "UTC:" << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
	shvInfo() << "======================================================================================";
	shvInfo() << "Log tresholds:" << NecroLog::tresholdsLogInfo();
	//shvInfo() << NecroLog::instantiationInfo();
	shvInfo() << "--------------------------------------------------------------------------------------";

	FlatLineApp a(argc, argv, &cli_opts);

	MainWindow w;
	w.show();

	shvInfo() << "starting main thread event loop";
	ret = a.exec();
	shvInfo() << "main event loop exit code:" << ret;
	shvInfo() << "bye ...";

	return ret;
}

