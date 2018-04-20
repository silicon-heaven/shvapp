#pragma once

#include <QCoreApplication>

class AppCliOptions;
class RevitestDevice;
namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}

class RevitestApp : public QCoreApplication
{
	Q_OBJECT
private:
	using Super = QCoreApplication;
public:
	RevitestApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~RevitestApp() Q_DECL_OVERRIDE;
private:
	//void onBrokerConnectedChanged(bool is_connected);
	//void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
private:
	RevitestDevice *m_revitest;
	shv::iotqt::rpc::DeviceConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;
};
