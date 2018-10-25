#pragma once

#include <shv/iotqt/rpc/deviceconnection.h>

class AppCliOptions : public shv::iotqt::rpc::DeviceAppCliOptions
{
	Q_OBJECT
private:
	using Super = shv::iotqt::rpc::DeviceAppCliOptions;
public:
	AppCliOptions(QObject *parent = NULL);
	~AppCliOptions() Q_DECL_OVERRIDE {}

	CLIOPTION_GETTER_SETTER2(QString, "sysfs.rootDir", s, setS, ysFsRootDir)
	CLIOPTION_GETTER_SETTER2(QString, "app.connStatusFile", c, setC, onnStatusFile)
	CLIOPTION_GETTER_SETTER2(QString, "app.sitesUrl", s, setS, itesUrl)
	CLIOPTION_GETTER_SETTER2(int, "app.connStatusUpdateInterval", c, setC, onnStatusUpdateInterval)

};

