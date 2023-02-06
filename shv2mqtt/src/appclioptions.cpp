#include "appclioptions.h"

AppCliOptions::AppCliOptions()
{
	addOption("mqtt.subscribePaths").setType(shv::chainpack::RpcValue::Type::String)
		.setComment("Can be set only from a config file");
	addOption("app.mqttHostname").setType(shv::chainpack::RpcValue::Type::String).setNames("--mqtt-hostname")
		.setComment("Hostname of the MQTT broker to connect to").setDefaultValue("localhost");
	addOption("app.mqttPort").setType(shv::chainpack::RpcValue::Type::UInt).setNames("--mqtt-port")
		.setComment("Port of the MQTT broker to connect to").setDefaultValue(1883);
	addOption("app.mqttUser").setType(shv::chainpack::RpcValue::Type::String).setNames("--mqtt-user")
		.setComment("User for the MQTT broker");
	addOption("app.mqttPassword").setType(shv::chainpack::RpcValue::Type::String).setNames("--mqtt-password")
		.setComment("Password for the MQTT broker");
	addOption("app.mqttClientId").setType(shv::chainpack::RpcValue::Type::String).setNames("--mqtt-client-id")
		.setComment("Client ID for the MQTT broker");
	addOption("app.mqttVersion").setType(shv::chainpack::RpcValue::Type::String).setNames("--mqtt-version")
		.setComment("MQTT version to use: 'mqtt5', 'mqtt311, 'mqtt31''").setDefaultValue("mqtt5");;
}
