#ifndef APPCLIOPTIONS_H
#define APPCLIOPTIONS_H

#include <shv/iotqt/rpc/clientappclioptions.h>

class AppCliOptions : public shv::iotqt::rpc::ClientAppCliOptions
{
	using Super = shv::iotqt::rpc::ClientAppCliOptions;
public:
	AppCliOptions();
};

#endif // APPCLIOPTIONS_H
