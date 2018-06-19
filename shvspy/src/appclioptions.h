#pragma once

#include <shv/coreqt/utils/clioptions.h>

#include <QSet>

class AppCliOptions : public shv::coreqt::utils::ConfigCLIOptions
{
	Q_OBJECT
private:
	using Super = shv::coreqt::utils::ConfigCLIOptions;

	CLIOPTION_GETTER_SETTER2(QString, "broker.subscribtions", s, setS, ubscriptions)
public:
	AppCliOptions(QObject *parent = NULL);
	~AppCliOptions() Q_DECL_OVERRIDE {}
};

