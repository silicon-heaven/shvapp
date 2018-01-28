#include "revitestapp.h"
#include "appclioptions.h"

#include <shv/core/utils.h>

#include <shv/coreqt/log.h>

#include <QTextStream>
#include <QTranslator>
#include <QDateTime>

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("Elektroline");
	QCoreApplication::setOrganizationDomain("elektroline.cz");
	QCoreApplication::setApplicationName("shvagent");
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

	shvInfo() << "======================================================================================";
	shvInfo() << "Starting revidestdevice, PID:" << QCoreApplication::applicationPid() << "build:" << __DATE__ << __TIME__;
#ifdef GIT_COMMIT
	shvInfo() << "GIT commit:" << SHV_EXPAND_AND_QUOTE(GIT_COMMIT);
#endif
	shvInfo() << QDateTime::currentDateTime().toString(Qt::ISODate).toStdString() << "UTC:" << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
	shvInfo() << "======================================================================================";
	shvInfo() << "Log tresholds:" << NecroLog::tresholdsLogInfo();
	shvInfo() << NecroLog::instantiationInfo();
	shvInfo() << "--------------------------------------------------------------------------------------";

	RevitestApp a(argc, argv, &cli_opts);

	QString lc_name = cli_opts.locale();
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

