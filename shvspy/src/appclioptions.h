#pragma once

#include <shv/iotqt/rpc/deviceconnection.h>

#include <QSet>

class AppCliOptions : public shv::iotqt::rpc::DeviceAppCliOptions
{
	Q_OBJECT
private:
	using Super = shv::iotqt::rpc::DeviceAppCliOptions;

	CLIOPTION_GETTER_SETTER2(QString, "protocol.type", p, setP, rotocolType)
	CLIOPTION_GETTER_SETTER2(QString, "broker.subscribtions", s, setS, ubscriptions)
public:
	AppCliOptions(QObject *parent = NULL);
	~AppCliOptions() Q_DECL_OVERRIDE {}
};

