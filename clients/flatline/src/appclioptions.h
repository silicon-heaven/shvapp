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
};

#endif // APPCLIOPTIONS_H
