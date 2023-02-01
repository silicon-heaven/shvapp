#include "shvrshapp.h"
#include "appclioptions.h"

#include <shv/chainpack/rpcmessage.h>

#include <shv/core/utils.h>

#include <shv/coreqt/log.h>
#include <shv/coreqt/utils.h>

#include <QTextStream>
#include <QTranslator>
#include <QDateTime>

#include <iostream>
#include <termios.h>
#include <unistd.h>

/* Place terminal referred to by 'fd' in raw mode (noncanonical mode
   with all input and output processing disabled). Return 0 on success,
   or -1 on error. If 'prevTermios' is non-NULL, then use the buffer to
   which it points to return the previous terminal settings. */

int ttySetRaw(int fd, struct termios *prev_termios)
{
	struct termios t;

	if (tcgetattr(fd, &t) == -1)
		return -1;

	if (prev_termios != NULL)
		*prev_termios = t;
	//return 0;

	/* Noncanonical mode, disable signals, extended input processing, and echoing */
	t.c_lflag &= ~(ISIG | IEXTEN);
	t.c_lflag &= ~ECHO;
	t.c_lflag &= ~ICANON;

	/* Disable special handling of CR, NL, and BREAK.
	   No 8th-bit stripping or parity error handling.
	   Disable START/STOP output flow control. */
	//t.c_iflag &= ~(BRKINT | ICRNL | IGNBRK | IGNCR | INLCR | INPCK | ISTRIP | IXON | PARMRK);
	t.c_iflag &= ~(BRKINT | IGNBRK | INPCK | ISTRIP | IXON | PARMRK);
	t.c_iflag &= ~(ICRNL | IGNCR | INLCR );

	t.c_oflag &= ~OPOST;                /* Disable all output processing */

	t.c_cc[VMIN] = 1;                   /* Character-at-a-time input */
	t.c_cc[VTIME] = 0;                  /* with blocking */

	if (tcsetattr(fd, TCSAFLUSH, &t) == -1)
		return -1;

	return 0;
}

int main(int argc, char *argv[])
{
	// call something from shv::coreqt to avoid linker error:
	// error while loading shared libraries: libshvcoreqt.so.1: cannot open shared object file: No such file or directory
	shv::coreqt::Utils::qVariantToRpcValue(QVariant());

	QCoreApplication::setOrganizationName("Elektroline");
	QCoreApplication::setOrganizationDomain("elektroline.cz");
	QCoreApplication::setApplicationName("shvrsh");
	QCoreApplication::setApplicationVersion("0.0.1");

	std::vector<std::string> shv_args = NecroLog::setCLIOptions(argc, argv);
	QStringList args;
	for(const std::string &arg : shv_args)
		args << QString::fromStdString(arg);

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

	shv::chainpack::RpcMessage::registerMetaTypes();

	struct termios tty_orig;
	if (ttySetRaw(STDIN_FILENO, &tty_orig) == -1) {
		shvError() << "ttySetRaw error";
		return EXIT_FAILURE;
	}

	shvInfo() << "======================================================================================";
	shvInfo() << "Starting SHV Remote Exec, PID:" << QCoreApplication::applicationPid() << "ver:" << QCoreApplication::applicationVersion();
#ifdef GIT_COMMIT
	shvInfo() << "GIT commit:" << SHV_EXPAND_AND_QUOTE(GIT_COMMIT);
#endif
	shvInfo() << QDateTime::currentDateTime().toString(Qt::ISODate).toStdString() << "UTC:" << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
	shvInfo() << "======================================================================================";
	shvInfo() << "Log tresholds:" << NecroLog::thresholdsLogInfo();
	shvInfo() << "--------------------------------------------------------------------------------------";

	ShvRshApp a(argc, argv, &cli_opts);

	shvInfo() << "starting main thread event loop";
	ret = a.exec();
	shvInfo() << "main event loop exit code:" << ret;
	shvInfo() << "bye ...";

	if (tcsetattr(STDIN_FILENO, TCSANOW, &tty_orig) == -1)
		shvError() << "tcsetattr error";

	return ret;
}

