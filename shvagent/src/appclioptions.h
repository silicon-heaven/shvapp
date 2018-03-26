#pragma once

#include <shv/iotqt/rpc/clientappclioptions.h>

#include <QSet>

class AppCliOptions : public shv::iotqt::rpc::ClientAppCliOptions
{
	Q_OBJECT
private:
	using Super = shv::iotqt::rpc::ClientAppCliOptions;
public:
	AppCliOptions(QObject *parent = NULL);
	~AppCliOptions() Q_DECL_OVERRIDE {}

	CLIOPTION_GETTER_SETTER2(QString, "sysfs.rootDir", s, setS, ysFsRootDir)
};

