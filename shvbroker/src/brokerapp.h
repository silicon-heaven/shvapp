#pragma once

#include "appclioptions.h"

#include <shv/chainpack/rpcvalue.h>

#include <QCoreApplication>
#include <QDateTime>

class QSocketNotifier;

namespace shv { namespace iotqt { class ShvNodeTree; }}
//namespace shv { namespace chainpack { class RpcMessage; }}
namespace rpc { class TcpServer; }

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
public:
	//rpc::TcpServer* tcpServer() {return m_tcpServer;}

	Q_SIGNAL void sqlServerConnected();
private:
	void lazyInit();

	QString serverProfile(); // unified access via Globals::serverProfile()
	void startTcpServer();
	//Q_SLOT void reconnectSqlServer();
	Q_SLOT void onSqlServerError(const QString &err_mesg);
	Q_SLOT void onSqlServerConnected();
	//Q_SLOT void reloadServices();

	void onRpcDataReceived(const shv::chainpack::RpcValue::MetaData &meta, const std::string &data);
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

