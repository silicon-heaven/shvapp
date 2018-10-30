#pragma once

#include <QCoreApplication>
#include <vector>

class AppCliOptions;
class RevitestDevice;
namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}
namespace shv { namespace iotqt { namespace utils { class FileShvJournal; struct ShvJournalEntry; }}}

class RevitestApp : public QCoreApplication
{
	Q_OBJECT
private:
	using Super = QCoreApplication;
public:
	RevitestApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~RevitestApp() Q_DECL_OVERRIDE;

	static constexpr size_t LUB_CNT = 27;
	static RevitestApp *instance();

	AppCliOptions* cliOptions() {return m_cliOptions;}

	shv::iotqt::utils::FileShvJournal *shvJournal() {return m_shvJournal;}
private:
	//void onBrokerConnectedChanged(bool is_connected);
	//void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void getSnapshot(std::vector<shv::iotqt::utils::ShvJournalEntry> &snapshot);
private:
	RevitestDevice *m_revitest;
	shv::iotqt::rpc::DeviceConnection *m_rpcConnection = nullptr;
	shv::iotqt::utils::FileShvJournal *m_shvJournal = nullptr;
	AppCliOptions* m_cliOptions;
};
