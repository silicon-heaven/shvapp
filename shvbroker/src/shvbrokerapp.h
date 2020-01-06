#pragma once

#include "appclioptions.h"

#include <shv/broker/brokerapp.h>

class ShvBrokerApp : public shv::broker::BrokerApp
{
	Q_OBJECT
private:
	using Super = shv::broker::BrokerApp;
public:
	ShvBrokerApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~ShvBrokerApp() Q_DECL_OVERRIDE;

	QString versionString() const;

	AppCliOptions* cliOptions() override;

	static ShvBrokerApp* instance() {return qobject_cast<ShvBrokerApp*>(Super::instance());}
};

