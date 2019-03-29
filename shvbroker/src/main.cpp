#include "brokerapp.h"
#include "appclioptions.h"
#include "utils/network.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/tunnelctl.h>

#include <shv/core/utils.h>

#include <shv/coreqt/log.h>

#include <QTextStream>
#include <QTranslator>
#include <QHostAddress>

#include <iostream>

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("Elektroline");
	QCoreApplication::setOrganizationDomain("elektroline.cz");
	QCoreApplication::setApplicationName("shvbroker");
	QCoreApplication::setApplicationVersion("0.0.1");

	NecroLog::registerTopic("Tunnel", "tunneling");
	NecroLog::registerTopic("Acl", "users and grants resolving");
	NecroLog::registerTopic("Access", "user access");
	NecroLog::registerTopic("Subscr", "subscriptions creation and propagation");
	NecroLog::registerTopic("SigRes", "signal resolution in client subscriptions");
	NecroLog::registerTopic("RpcMsg", "dump RPC messages");
	NecroLog::registerTopic("RpcRawMsg", "dump raw RPC messages"),
	NecroLog::registerTopic("RpcData", "dump RPC mesages as HEX data");

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
	shvInfo() << "Starting SHV BROKER server, PID:" << QCoreApplication::applicationPid() << "build:" << __DATE__ << __TIME__;
#ifdef GIT_COMMIT
	shvInfo() << "GIT commit:" << SHV_EXPAND_AND_QUOTE(GIT_COMMIT);
#endif
	shvInfo() << QDateTime::currentDateTime().toString(Qt::ISODate).toStdString() << "UTC:" << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
	shvInfo() << "======================================================================================";
	shvInfo() << "Log tresholds:" << NecroLog::tresholdsLogInfo();
	//shvInfo() << NecroLog::instantiationInfo();
	shvInfo() << "Primary IPv4 address:" << utils::Network::primaryIPv4Address().toString();
	shvInfo() << "Primary public IPv4 address:" << utils::Network::primaryPublicIPv4Address().toString();
	shvInfo() << "--------------------------------------------------------------------------------------";

	BrokerApp a(argc, argv, &cli_opts);

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

	shvInfo() << "starting main thread event loop";
	ret = a.exec();
	shvInfo() << "main event loop exit code:" << ret;
	shvInfo() << "EYAS server bye ...";

	return ret;
}

