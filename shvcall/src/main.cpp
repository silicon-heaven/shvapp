#include "application.h"
#include "appclioptions.h"

#include <shv/core/utils.h>
#include <shv/coreqt/log.h>

#include <iostream>

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("Elektroline");
	QCoreApplication::setOrganizationDomain("elektroline.cz");
	QCoreApplication::setApplicationName("shvcall");
	QCoreApplication::setApplicationVersion("0.1.0");

	char **argv_modified = new char*[(uint)(argc + 2)];
	bool has_debug_param = false;
	for (int i = 0; i < argc; ++i) {
		if (strcmp(argv[i], "-d") == 0) {
			has_debug_param = true;
		}
		argv_modified[i] = argv[i];
	}
	if (!has_debug_param) {
		argv_modified[argc] = new char[3];
		strcpy(argv_modified[argc++], "-d");
		argv_modified[argc] = new char[3];
		strcpy(argv_modified[argc++], ":E");
	}
	std::vector<std::string> shv_args = NecroLog::setCLIOptions(argc, argv_modified);

	AppCliOptions cli_opts;
	cli_opts.parse(shv_args);
	if (cli_opts.isParseError()) {
		for(const std::string &err : cli_opts.parseErrors()) {
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
	QString path;
	QString method;
	shv::chainpack::RpcValue params;

	Application a(argc, argv, &cli_opts);
	return a.exec();
}
