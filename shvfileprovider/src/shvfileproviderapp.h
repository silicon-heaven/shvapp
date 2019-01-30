#pragma once

#include <shv/iotqt/node/shvnode.h>
#include "fileproviderlocalfsnode.h"

#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class AppCliOptions;
class QTimer;

namespace shv { namespace chainpack { class RpcMessage; }}
namespace shv { namespace iotqt { namespace rpc { class DeviceConnection; }}}
namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}

class ShvFileProviderApp : public QCoreApplication
{
	Q_OBJECT
private:
	using Super = QCoreApplication;
public:
	ShvFileProviderApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~ShvFileProviderApp() Q_DECL_OVERRIDE;

	static ShvFileProviderApp *instance();
	shv::iotqt::rpc::DeviceConnection *rpcConnection() const {return m_rpcConnection;}

	AppCliOptions* cliOptions() {return m_cliOptions;}
private:
	void onBrokerConnectedChanged(bool is_connected);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	void onNetworkManagerFinished(QNetworkReply *reply);

	shv::iotqt::rpc::DeviceConnection *m_rpcConnection = nullptr;
	AppCliOptions* m_cliOptions;
	FileProviderLocalFsNode *m_root = nullptr;
	bool m_isBrokerConnected = false;
};

