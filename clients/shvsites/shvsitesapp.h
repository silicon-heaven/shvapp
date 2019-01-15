#ifndef SHVSITESAPP_H
#define SHVSITESAPP_H

//#include <shv/chainpack/rpcvalue.h>

#include <QGuiApplication>

class AppCliOptions;
class SitesModel;
namespace shv { namespace iotqt { namespace rpc { class ClientConnection; }}}
namespace shv { namespace chainpack { class RpcMessage; class RpcValue; }}

class ShvSitesApp : public QGuiApplication
{
	Q_OBJECT

	using Super = QGuiApplication;
public:
	ShvSitesApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~ShvSitesApp() Q_DECL_OVERRIDE;

	static ShvSitesApp *instance();
	SitesModel *sitesModel() const {return m_sitesModel;}
	shv::iotqt::rpc::ClientConnection *rpcConnection() const {return m_rpcConnection;}
	AppCliOptions* cliOptions() {return m_cliOptions;}
private:
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void onBrokerConnected(bool is_connected);
	void loadSitesModel();
	void loadSitesHelper(std::vector<std::string> &paths, const shv::chainpack::RpcValue &value);

private:
	shv::iotqt::rpc::ClientConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;
	SitesModel *m_sitesModel = nullptr;
};

#endif // SHVSITESAPP_H
