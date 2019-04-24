#pragma once

#include <QApplication>
#include <vector>

class AppCliOptions;
class RevitestDevice;
namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class ClientConnection; }}}
namespace shv { namespace iotqt { namespace utils { class FileShvJournal; struct ShvJournalEntry; }}}

class FlatLineApp : public QApplication
{
	Q_OBJECT
private:
	using Super = QApplication;
public:
	static const char *SIG_FASTCHNG;
public:
	FlatLineApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~FlatLineApp() Q_DECL_OVERRIDE;

	static FlatLineApp *instance();

	shv::iotqt::rpc::ClientConnection* rpcConnection() {return m_rpcConnection;}
	AppCliOptions* cliOptions() {return m_cliOptions;}
private:
	void onBrokerConnectedChanged(bool is_connected);
	//void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
private:
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;
};
