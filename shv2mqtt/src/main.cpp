#include "appclioptions.h"
#include "shv2mqttapp.h"

#include <shv/coreqt/log.h>
#include <shv/coreqt/utils.h>
#include <shv/iotqt/rpc/deviceconnection.h>

#include <QCoreApplication>

#include <iostream>

int main(int argc, char* argv[])
{
	QCoreApplication::setOrganizationName("Elektroline");
	QCoreApplication::setOrganizationDomain("elektroline.cz");
	QCoreApplication::setApplicationName("shv2mqtt");
	QCoreApplication::setApplicationVersion("0.0.1");

	std::vector<std::string> shv_args = NecroLog::setCLIOptions(argc, argv);

	int ret = 0;

	AppCliOptions cli_opts;
	cli_opts.parse(shv_args);
	if (cli_opts.isParseError()) {
		for (const std::string& err : cli_opts.parseErrors()) {
			shvError() << err;
		}
		return EXIT_FAILURE;
	}

	if (cli_opts.isAppBreak()) {
		if (cli_opts.isHelp()) {
			cli_opts.printHelp(std::cout);
		}
		return EXIT_SUCCESS;
	}

	for (const std::string& s : cli_opts.unusedArguments()) {
		shvWarning() << "Undefined argument:" << s;
	}

	if (!cli_opts.loadConfigFile()) {
		return EXIT_FAILURE;
	}

	shv::chainpack::RpcMessage::registerMetaTypes();

	shvInfo() << "======================================================================================";
	shvInfo() << "Starting shv2mqtt bridge, PID:" << QCoreApplication::applicationPid() << "build:" << __DATE__ << __TIME__;
#ifdef GIT_COMMIT
	shvInfo() << "GIT commit:" << SHV_EXPAND_AND_QUOTE(GIT_COMMIT);
#endif
	shvInfo() << QDateTime::currentDateTime().toString(Qt::ISODate).toStdString() << "UTC:" << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
	shvInfo() << "======================================================================================";
	shvInfo() << "Log tresholds:" << NecroLog::thresholdsLogInfo();
	shvInfo() << "--------------------------------------------------------------------------------------";

	Shv2MqttApp a(argc, argv, &cli_opts, new shv::iotqt::rpc::DeviceConnection(nullptr));

	shvInfo() << "starting main thread event loop";
	ret = a.exec();
	shvInfo() << "main event loop exit code:" << ret;
	shvInfo() << "bye ...";

	return ret;
}
