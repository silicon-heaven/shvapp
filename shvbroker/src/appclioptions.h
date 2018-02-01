#pragma once

#include <shv/coreqt/utils/clioptions.h>

#include <QSet>

class AppCliOptions : public shv::coreqt::utils::ConfigCLIOptions
{
	Q_OBJECT
private:
	using Super = shv::coreqt::utils::ConfigCLIOptions;
public:
	AppCliOptions(QObject *parent = NULL);
	~AppCliOptions() Q_DECL_OVERRIDE {}

	CLIOPTION_GETTER_SETTER(QString, l, setL, ocale)
	CLIOPTION_GETTER_SETTER(QString, p, setP, rofile)
	CLIOPTION_GETTER_SETTER2(int, "server.port", s, setS, erverPort)
	CLIOPTION_GETTER_SETTER2(QString, "sql.host", s, setS, qlHost)
	CLIOPTION_GETTER_SETTER2(int, "sql.port", s, setS, qlPort)
	CLIOPTION_GETTER_SETTER2(bool, "debug.echoEnabled", is, set, EchoEnabled)
};

