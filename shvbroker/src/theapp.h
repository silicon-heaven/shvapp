#pragma once

#include "appclioptions.h"

#include <QCoreApplication>
#include <QDateTime>

class QSocketNotifier;

class TheApp : public QCoreApplication
{
	Q_OBJECT
private:
	typedef QCoreApplication Super;
public:
	TheApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~TheApp() Q_DECL_OVERRIDE;

	QString versionString() const;

	AppCliOptions* cliOptions() {return m_cliOptions;}
	//sql::SqlConnector *sqlConnector();

	static TheApp* instance() {return qobject_cast<TheApp*>(Super::instance());}
public:
	//rpc::TcpServer* tcpServer() {return m_tcpServer;}

	Q_SIGNAL void sqlServerConnected();
private:
	void lazyInit();

	QString serverProfile(); // unified access via Globals::serverProfile()
	//rpc::TcpServer* startTcpServer();
	//Q_SLOT void reconnectSqlServer();
	Q_SLOT void onSqlServerError(const QString &err_mesg);
	Q_SLOT void onSqlServerConnected();
	//Q_SLOT void reloadServices();

private:
	AppCliOptions *m_cliOptions;
	/*
	sql::SqlConnector *m_sqlConnector = nullptr;
	rpc::TcpServer *m_tcpServer = nullptr;
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

