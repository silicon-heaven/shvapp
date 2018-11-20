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

	CLIOPTION_GETTER_SETTER2(QString, "fs.rootDir", f, setF, sRootDir)
};

