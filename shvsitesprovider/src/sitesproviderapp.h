#pragma once

#include <shv/iotqt/node/shvnode.h>
#include <shv/chainpack/rpcmessage.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QMap>


class AppCliOptions;
class AppRootNode;

namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}
namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}

class SitesProviderApp : public QCoreApplication
{
	Q_OBJECT
	using Super = QCoreApplication;

public:
	SitesProviderApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~SitesProviderApp() Q_DECL_OVERRIDE;

	static SitesProviderApp *instance();
	shv::iotqt::rpc::DeviceConnection *rpcConnection() const { return m_rpcConnection; }
	AppCliOptions* cliOptions() {return m_cliOptions;}
	QString remoteSitesUrl() const;
	QString remoteSitesUrlScheme() const;

private:
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	shv::iotqt::rpc::DeviceConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;

	shv::iotqt::node::ShvNodeTree *m_shvTree = nullptr;
	AppRootNode *m_rootNode;
};

