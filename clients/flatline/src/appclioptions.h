#ifndef APPCLIOPTIONS_H
#define APPCLIOPTIONS_H

#include <shv/iotqt/rpc/clientappclioptions.h>

class AppCliOptions : public shv::iotqt::rpc::ClientAppCliOptions
{
	using Super = shv::iotqt::rpc::ClientAppCliOptions;
public:
	AppCliOptions();

	CLIOPTION_GETTER_SETTER2(std::string, "device.shvPath", d, setD, eviceShvPath)
	CLIOPTION_GETTER_SETTER2(std::string, "device.logFile", l, setL, ogFile)
	CLIOPTION_GETTER_SETTER2(bool, "app.testMode", is, set, TestMode)
};

#endif // APPCLIOPTIONS_H
