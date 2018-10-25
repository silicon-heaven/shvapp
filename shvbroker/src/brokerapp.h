#pragma once

#include "appclioptions.h"

#include <shv/iotqt/node/shvnode.h>

#include <shv/chainpack/rpcvalue.h>

#include <QCoreApplication>
#include <QDateTime>

class QSocketNotifier;

namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}
namespace shv { namespace chainpack { class RpcNotify; }}
namespace rpc { class TcpServer; class ServerConnection; }

class BrokerApp : public QCoreApplication
{
	Q_OBJECT
private:
	typedef QCoreApplication Super;
public:
	BrokerApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~BrokerApp() Q_DECL_OVERRIDE;

	QString versionString() const;

	AppCliOptions* cliOptions() {return m_cliOptions;}
	//sql::SqlConnector *sqlConnector();

	static BrokerApp* instance() {return qobject_cast<BrokerApp*>(Super::instance());}

	void onClientLogin(int connection_id);
	void onRpcDataReceived(int connection_id, shv::chainpack::RpcValue::MetaData &&meta, std::string &&data);

	void createSubscription(int client_id, const std::string &path, const std::string &method);

	rpc::TcpServer* tcpServer();
	rpc::ServerConnection* clientById(int client_id);

	//Q_SIGNAL void sqlServerConnected();
	std::string fstabCpon();
	void saveFstabCpon(const std::string &cpon);
	//void reloadFstab();
private:
	void lazyInit();

	QString serverProfile(); // unified access via Globals::serverProfile()
	void startTcpServer();
	//Q_SLOT void reconnectSqlServer();
	//Q_SLOT void onSqlServerError(const QString &err_mesg);
	//Q_SLOT void onSqlServerConnected();
	//Q_SLOT void reloadServices();
	std::string mountPointForDevice(const shv::chainpack::RpcValue &device_id);

	void onRootNodeSendRpcMesage(const shv::chainpack::RpcMessage &msg);

	void sendNotifyToSubscribers(int sender_connection_id, const std::string &shv_path, const std::string &method, const shv::chainpack::RpcValue &params);
	void sendNotifyToSubscribers(int sender_connection_id, const shv::chainpack::RpcValue::MetaData &meta_data, const std::string &data);

	static std::string brokerClientPath(int client_id);
private:
	AppCliOptions *m_cliOptions;
	rpc::TcpServer *m_tcpServer = nullptr;
	shv::iotqt::node::ShvNodeTree *m_deviceTree = nullptr;
	shv::chainpack::RpcValue m_fstab;
	/*
	sql::SqlConnector *m_sqlConnector = nullptr;
	QTimer *m_sqlConnectionWatchDog;
	*/
#ifdef Q_OS_UNIX
private:
	// Unix signal handlers.
	// You can't call Qt functions from Unix signal handlers,
	// but you can write to socket
	void installUnixSignalHandlers();
	Q_SLOT void handleSigTerm();
	static void sigTermHandler(int);

	static int m_sigTermFd[2];
	QSocketNotifier *m_snTerm = nullptr;
#endif
};

