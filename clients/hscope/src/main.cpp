#include "version.h"
#include "hscopeapp.h"
#include "appclioptions.h"

#include <shv/core/utils.h>

#include <shv/coreqt/log.h>

#include <QTextStream>
#include <QTranslator>
#include <QDateTime>
#include <QDir>

#include <fstream>
#include <iostream>

namespace {

NecroLog::MessageHandler old_message_handler;

std::ofstream log_file_stream;
/*
void copy_to_file_message_handler(NecroLog::Level level, const NecroLog::LogContext &context, const std::string &msg)
{
	//std::fstream file(HScopeApp::logFilePath(), std::ios::out | std::ios::binary | std::ios::app);
	std::ostream &os = log_file_stream;
	NecroLog::writeWithDefaultFormat(os, false, level, context, msg);

	old_message_handler(level, context, msg);
}

void init_log_environment()
{
	std::string log_file_path = HScopeApp::logFilePath();
	{
		// keep log file smaller than max_size
		constexpr unsigned max_size = 1024 * 1024;
		constexpr unsigned min_size = max_size / 2;
		ssize_t size = 0;
		{
			std::ifstream f1(log_file_path, std::ios::binary);
			f1.seekg (0, std::ios::end);
			size = f1.tellg();
		}
		if(size > max_size) {
			std::string copy_fn = log_file_path + ".copy";
			std::rename(log_file_path.c_str(), copy_fn.c_str());
			std::ifstream f1(copy_fn, std::ios::binary);
			f1.seekg (size - min_size);
			std::ofstream f2(log_file_path, std::ios::binary);
			f2 << f1.rdbuf();
			std::remove(copy_fn.c_str());
		}
	}
	log_file_stream.open(log_file_path, std::ios::binary | std::ios::out | std::ios::app);
	old_message_handler = NecroLog::setMessageHandler(copy_to_file_message_handler);
}
*/
}

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("Elektroline");
	QCoreApplication::setOrganizationDomain("elektroline.cz");
	QCoreApplication::setApplicationName("hscope");
	QCoreApplication::setApplicationVersion(APP_VERSION);

	//init_log_environment();

	std::vector<std::string> shv_args = NecroLog::setCLIOptions(argc, argv);

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
	shvInfo() << "Starting BfsView, PID:" << QCoreApplication::applicationPid() << "build:" << __DATE__ << __TIME__;
#ifdef GIT_COMMIT
	shvInfo() << "GIT commit:" << SHV_EXPAND_AND_QUOTE(GIT_COMMIT);
#endif
	shvInfo() << QDateTime::currentDateTime().toString(Qt::ISODate).toStdString() << "UTC:" << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
	shvInfo() << "======================================================================================";
	shvInfo() << "Log tresholds:" << NecroLog::tresholdsLogInfo();
	shvInfo() << "--------------------------------------------------------------------------------------";

	HScopeApp app(argc, argv, &cli_opts);

	shvInfo() << "starting main thread event loop";
	ret = app.exec();
	shvInfo() << "main event loop exit code:" << ret;
	shvInfo() << "bye ...";

	return ret;
}

