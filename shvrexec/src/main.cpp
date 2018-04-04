#include "shvrexecapp.h"
#include "appclioptions.h"

#include <shv/core/utils.h>

#include <shv/coreqt/log.h>

#include <QTextStream>
#include <QTranslator>
#include <QDateTime>

#include <iostream>
#include <termios.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	//for (int i = 0; i < argc; ++i)
	//	std::cerr << i << " " << argv[i] << std::endl;

	QCoreApplication::setOrganizationName("Elektroline");
	QCoreApplication::setOrganizationDomain("elektroline.cz");
	QCoreApplication::setApplicationName("shvrexec");
	QCoreApplication::setApplicationVersion("0.0.1");

	std::vector<std::string> shv_args = NecroLog::setCLIOptions(argc, argv);
	QStringList args;
	for(const std::string &arg : shv_args)
		args << QString::fromStdString(arg);

	int ret = 0;

	AppCliOptions cli_opts;
	cli_opts.parse(args);
	if(cli_opts.isParseError()) {
		foreach(QString err, cli_opts.parseErrors())
			shvError() << err.toStdString();
		return EXIT_FAILURE;
	}
	if(cli_opts.isAppBreak()) {
		if(cli_opts.isHelp()) {
			QTextStream ts(stdout);
			cli_opts.printHelp(ts);
		}
		return EXIT_SUCCESS;
	}
	foreach(QString s, cli_opts.unusedArguments()) {
		shvWarning() << "Undefined argument:" << s;
	}

	if(!cli_opts.loadConfigFile()) {
		return EXIT_FAILURE;
	}
	/*
	if(!cli_opts.password_isset()) {
		std::string password;
		const bool is_tty = ::isatty(STDERR_FILENO);
		if(is_tty) {
			std::cout << "password: ";
			termios oldt;
			::tcgetattr(STDIN_FILENO, &oldt);
			termios newt = oldt;
			newt.c_lflag &= ~ECHO;
			::tcsetattr(STDIN_FILENO, TCSANOW, &newt);
			std::getline(std::cin, password);
			::tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
			std::cout << "\n";
		}
		else {
			std::getline(std::cin, password);
		}
		cli_opts.setPassword(QString::fromStdString(password));
	}
	*/
	shvInfo() << "======================================================================================";
	shvInfo() << "Starting SHV Remote Exec, PID:" << QCoreApplication::applicationPid() << "build:" << __DATE__ << __TIME__;
#ifdef GIT_COMMIT
	shvInfo() << "GIT commit:" << SHV_EXPAND_AND_QUOTE(GIT_COMMIT);
#endif
	shvInfo() << QDateTime::currentDateTime().toString(Qt::ISODate).toStdString() << "UTC:" << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
	shvInfo() << "======================================================================================";
	shvInfo() << "Log tresholds:" << NecroLog::tresholdsLogInfo();
	shvInfo() << "--------------------------------------------------------------------------------------";

	ShvRExecApp a(argc, argv, &cli_opts);

	shvInfo() << "starting main thread event loop";
	ret = a.exec();
	shvInfo() << "main event loop exit code:" << ret;
	shvInfo() << "bye ...";

	return ret;
}

