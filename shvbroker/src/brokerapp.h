#pragma once

#include "appclioptions.h"

#include <shv/iotqt/shvnode.h>

#include <shv/chainpack/rpcvalue.h>

#include <QCoreApplication>
#include <QDateTime>

class QSocketNotifier;

namespace shv { namespace iotqt { class ShvNodeTree; }}
//namespace shv { namespace chainpack { class RpcMessage; }}
namespace rpc { class TcpServer; class ServerConnection; }

class ConnectionNode : public shv::iotqt::ShvNode
{
	Q_OBJECT
private:
	using Super = shv::iotqt::ShvNode;
public:
	ConnectionNode(rpc::ServerConnection *connection, QObject *parent = nullptr);
	rpc::ServerConnection * connection() const {return m_connection;}
private:
	rpc::ServerConnection * m_connection = nullptr;
};

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

	bool onClientLogin(int connection_id);
	void onRpcDataReceived(const shv::chainpack::RpcValue::MetaData &meta, const std::string &data);
public:
	rpc::TcpServer* tcpServer();

	Q_SIGNAL void sqlServerConnected();
private:
	void lazyInit();

	QString serverProfile(); // unified access via Globals::serverProfile()
	void startTcpServer();
	//Q_SLOT void reconnectSqlServer();
	Q_SLOT void onSqlServerError(const QString &err_mesg);
	Q_SLOT void onSqlServerConnected();
	//Q_SLOT void reloadServices();
private:
	AppCliOptions *m_cliOptions;
	rpc::TcpServer *m_tcpServer = nullptr;
	shv::iotqt::ShvNodeTree *m_deviceTree = nullptr;
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
	static void sigTermHandler(int);
	Q_SLOT void handleSigTerm();

	static int m_sigTermFd[2];
	QSocketNotifier *m_snTerm = nullptr;
#endif
};

