#pragma once

#include <shv/iotqt/client/consoleapplication.h>

class AppCliOptions;
namespace shv { namespace chainpack { class RpcMessage; }}

class TheApp : public shv::iotqt::client::ConsoleApplication
{
	Q_OBJECT
private:
	using Super = shv::iotqt::client::ConsoleApplication;
public:
	TheApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~TheApp() Q_DECL_OVERRIDE;
private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
};

