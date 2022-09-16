#pragma once

#include <shv/chainpack/rpcvalue.h>

#include <QCoreApplication>

class AppCliOptions;

namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}
namespace shv { namespace core { namespace utils { class ShvFileJournal; struct ShvJournalEntry; }}}

class Application : public QCoreApplication
{
	Q_OBJECT
private:
	using Super = QCoreApplication;
public:
	Application(int &argc, char **argv, AppCliOptions* cli_opts);
	~Application() Q_DECL_OVERRIDE;

	static Application *instance();

	AppCliOptions* cliOptions() {return m_cliOptions;}

private:
	void onBrokerConnectedChanged(bool is_connected);
	//void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
private:
	shv::iotqt::rpc::DeviceConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;
};
