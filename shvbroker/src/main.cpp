#include "shvbrokerapp.h"
#include "version.h"
#include "appclioptions.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/utils/network.h>

#include <QHostAddress>

#include <iostream>

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("Elektroline");
	QCoreApplication::setOrganizationDomain("elektroline.cz");
	QCoreApplication::setApplicationName("shvbroker");
	QCoreApplication::setApplicationVersion(APP_VERSION);

	ShvBrokerApp::registerLogTopics();

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
		shvError() << "Undefined argument:" << s;
	}

	if(!cli_opts.loadConfigFile()) {
		return EXIT_FAILURE;
	}

	shv::chainpack::RpcMessage::registerMetaTypes();

	shvInfo() << "======================================================================================";
	shvInfo() << "Starting SHV BROKER server ver:" << APP_VERSION
				 << "PID:" << QCoreApplication::applicationPid()
				 << "build:" << __DATE__ << __TIME__;
#ifdef GIT_COMMIT
	shvInfo() << "GIT commit:" << SHV_EXPAND_AND_QUOTE(GIT_COMMIT);
#endif
	shvInfo() << QDateTime::currentDateTime().toString(Qt::ISODate).toStdString() << "UTC:" << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
	shvInfo() << "======================================================================================";
	shvInfo() << "Log tresholds:" << NecroLog::tresholdsLogInfo();
	shvInfo() << "Config dir:" << cli_opts.configDir();
	if(cli_opts.isSqlConfigEnabled())
		shvInfo() << "SQL config database" << cli_opts.sqlConfigDatabase() << "driver" << cli_opts.sqlConfigDriver();
	shvInfo() << "Primary IPv4 address:" << shv::iotqt::utils::Network::primaryIPv4Address().toString();
	shvInfo() << "Primary public IPv4 address:" << shv::iotqt::utils::Network::primaryPublicIPv4Address().toString();
	shvInfo() << "--------------------------------------------------------------------------------------";

	ShvBrokerApp a(argc, argv, &cli_opts);
#if 0
	QString lc_name = QString::fromStdString(cli_opts.locale());
	{
		if (lc_name.isEmpty() || lc_name == QStringLiteral("system")) {
			lc_name = QLocale::system().name();
		}
		QString app_translations_path = QCoreApplication::applicationDirPath() + QStringLiteral("/translations");
		//QString qt_translations_path = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
		QString qt_translations_path = app_translations_path;
		shvInfo() << "Loading translations for:" << lc_name;
		{
			QTranslator *qt_translator = new QTranslator(&a);
			QString tr_name = "qt_" + lc_name;
			bool ok = qt_translator->load(tr_name, qt_translations_path);
			if (ok) {
				ok = a.installTranslator(qt_translator);
				shvInfo() << "Installing translator file:" << tr_name << " ... " << (ok ? "OK": "ERROR");
			}
			else {
				shvInfo() << "Erorr loading translator file:" << (qt_translations_path + '/' + tr_name);
			}
		}
		for (QString prefix : {"libqfcore", "libeyascore", "eyassrv"}) {
			QTranslator *qt_translator = new QTranslator(&a);
			QString tr_name = prefix + "." + lc_name;
			bool ok = qt_translator->load(tr_name, app_translations_path);
			if(ok) {
				ok = a.installTranslator(qt_translator);
				shvInfo() << "Installing translator file:" << tr_name << " ... " << (ok? "OK": "ERROR");
			}
			else {
				shvInfo() << "Erorr loading translator file:" << (app_translations_path + '/' + tr_name);
			}
		}
	}
#endif
	shvInfo() << "starting main thread event loop";
	ret = a.exec();
	shvInfo() << "main event loop exit code:" << ret;
	shvInfo() << "EYAS server bye ...";

	return ret;
}

