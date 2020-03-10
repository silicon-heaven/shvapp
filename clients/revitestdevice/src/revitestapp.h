#pragma once

#include <shv/chainpack/rpcvalue.h>

#include <QCoreApplication>

#include <vector>

class AppCliOptions;
class RevitestDevice;
namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}
namespace shv { namespace core { namespace utils { class ShvFileJournal; struct ShvJournalEntry; }}}

class RevitestApp : public QCoreApplication
{
	Q_OBJECT
private:
	using Super = QCoreApplication;
public:
	RevitestApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~RevitestApp() Q_DECL_OVERRIDE;

	static RevitestApp *instance();

	AppCliOptions* cliOptions() {return m_cliOptions;}

	shv::core::utils::ShvFileJournal *shvJournal() {return m_shvJournal;}
private:
	void onBrokerConnectedChanged(bool is_connected);
	//void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void getSnapshot(std::vector<shv::core::utils::ShvJournalEntry> &snapshot);

	void processShvCalls();
private:
	RevitestDevice *m_revitest;
	shv::iotqt::rpc::DeviceConnection *m_rpcConnection = nullptr;
	shv::core::utils::ShvFileJournal *m_shvJournal = nullptr;
	AppCliOptions* m_cliOptions;
	shv::chainpack::RpcValue::List m_shvCalls;
};
