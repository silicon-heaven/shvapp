#pragma once

#include <shv/iotqt/client/appclioptions.h>

#include <QSet>

class AppCliOptions : public shv::iotqt::client::AppCliOptions
{
	Q_OBJECT
private:
	using Super = shv::iotqt::client::AppCliOptions;
public:
	AppCliOptions(QObject *parent = NULL);
	~AppCliOptions() Q_DECL_OVERRIDE {}

	CLIOPTION_GETTER_SETTER2(int, "server.port", s, setS, erverPort)
	CLIOPTION_GETTER_SETTER2(QString, "server.host", s, setS, erverHost)
};

