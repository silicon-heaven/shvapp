#pragma once


#include <shv/broker/appclioptions.h>
#include <shv/broker/brokerapp.h>

class ShvBrokerApp : public shv::broker::BrokerApp
{
	Q_OBJECT
private:
	using Super = shv::broker::BrokerApp;
public:
	ShvBrokerApp(int &argc, char **argv, shv::broker::AppCliOptions* cli_opts);
	~ShvBrokerApp() Q_DECL_OVERRIDE;

	QString versionString() const;

	static ShvBrokerApp* instance() {return qobject_cast<ShvBrokerApp*>(Super::instance());}
};

