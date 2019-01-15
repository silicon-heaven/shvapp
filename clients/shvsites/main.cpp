#include "shvsitesapp.h"
#include "sitesmodel.h"
#include "appclioptions.h"

#include <shv/coreqt/log.h>

#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QSettings>

#include <iostream>

int main(int argc, char **argv)
{
	QCoreApplication::setOrganizationName("Elektroline");
	QCoreApplication::setOrganizationDomain("elektroline.cz");
	QCoreApplication::setApplicationName("shvplaces");
	QCoreApplication::setApplicationVersion("0.0.1");

	std::vector<std::string> shv_args = NecroLog::setCLIOptions(argc, argv);

	shvDebug() << "debug log test";
	shvInfo() << "info log test";
	shvWarning() << "warning log test";
	shvError() << "error log test";

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
	shvInfo() << "Starting shvsites, PID:" << QCoreApplication::applicationPid() << "build:" << __DATE__ << __TIME__;
#ifdef GIT_COMMIT
	shvInfo() << "GIT commit:" << SHV_EXPAND_AND_QUOTE(GIT_COMMIT);
#endif
	shvInfo() << QDateTime::currentDateTime().toString(Qt::ISODate).toStdString() << "UTC:" << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toStdString();
	shvInfo() << "======================================================================================";
	shvInfo() << "Log tresholds:" << NecroLog::tresholdsLogInfo();

	ShvSitesApp app(argc,argv, &cli_opts);

	SitesModel *model = app.sitesModel();
	//model.addPredator("shv/cz/plz/tram/switch/karlov", "Andi Karlovarska", GPS(49.7477831, 13.3783489));
	//model.addPredator("pol/ols/tram/switch/001", "001 Kosciuszki-Pilsudskiego", GPS(53.7769533, 20.4778597));
	//model.addPredator("pol/lub/tbus/discon/1101", "1101 Zajezdnia - Muzeum", GPS(51.2181944, 22.5546775));

	QQuickView view;
	view.setResizeMode(QQuickView::SizeRootObjectToView);
	QQmlContext *ctxt = view.rootContext();
	ctxt->setContextProperty("poiModel", model);
	view.setSource(QUrl(QStringLiteral("qrc:///map.qml")));

	QSettings settings;
	QRect g = settings.value("ui/mainWindow/geometry").toRect();
	//qDebug() << g;
	if(g.isValid()) {
		view.setGeometry(g);
	}
	else {
		view.setWidth(1024);
		view.setHeight(768);
	}

	view.show();

	int ret = app.exec();

	//qDebug() << view.geometry();
	settings.setValue("ui/mainWindow/geometry", view.geometry());

	return ret;
}
