#pragma once

#include <QCoreApplication>

class AppCliOptions;
namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class ClientConnection; }}}

class TheApp : public QCoreApplication
{
	Q_OBJECT
private:
	using Super = QCoreApplication;
public:
	TheApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~TheApp() Q_DECL_OVERRIDE;
private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
private:
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;
};

