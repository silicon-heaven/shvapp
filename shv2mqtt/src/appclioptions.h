#pragma once

#include <shv/iotqt/rpc/deviceappclioptions.h>

class AppCliOptions : public shv::iotqt::rpc::DeviceAppCliOptions
{
private:
	using Super = shv::iotqt::rpc::DeviceAppCliOptions;
public:
	AppCliOptions();
	CLIOPTION_GETTER_SETTER2(std::string, "app.subscribePath", s, setS, ubscribePath)
	CLIOPTION_GETTER_SETTER2(std::string, "app.mqttHostname", m, setM, qttHostname)
	CLIOPTION_GETTER_SETTER2(shv::chainpack::RpcValue::UInt, "app.mqttPort", m, setM, qttPort)
	CLIOPTION_GETTER_SETTER2(std::string, "app.mqttUser", m, setM, qttUser)
	CLIOPTION_GETTER_SETTER2(std::string, "app.mqttPassword", m, setM, qttPassword)
	CLIOPTION_GETTER_SETTER2(std::string, "app.mqttClientId", m, setM, qttClientId)
	CLIOPTION_GETTER_SETTER2(std::string, "app.mqttVersion", m, setM, qttVersion)
};
