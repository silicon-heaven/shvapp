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
	CLIOPTION_GETTER_SETTER2(QString, "user.name", u, setU, serName)
	CLIOPTION_GETTER_SETTER2(QString, "server.host", s, setS, erverHost)
	CLIOPTION_GETTER_SETTER2(int, "server.port", s, setS, erverPort)
	CLIOPTION_GETTER_SETTER2(int, "rpc.timeout", r, setR, pcTimeout)
};

