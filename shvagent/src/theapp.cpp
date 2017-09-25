#include "theapp.h"
#include "rpc/clientconnection.h"

#include <shv/coreqt/log.h>

#include <QSocketNotifier>
#include <QTimer>

#ifdef Q_OS_UNIX
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

//#define logOpcuaReceive qfCInfo("OpcuaReceive")

#ifdef Q_OS_UNIX
int TheApp::m_sigTermFd[2];
#endif

//static constexpr int SQL_RECONNECT_INTERVAL = 3000;

TheApp::TheApp(int &argc, char **argv, AppCliOptions *cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	shvInfo() << "creating SHV BROKER application object ver." << versionString();
#ifdef Q_OS_UNIX
	//syslog (LOG_INFO, "Server started");
	installUnixSignalHandlers();
#endif
	m_checkConnectedTimer = new QTimer(this);
	m_checkConnectedTimer->start(1000 * 10);
	connect(m_checkConnectedTimer, &QTimer::timeout, this, &TheApp::checkConnected);

	QTimer::singleShot(0, this, &TheApp::lazyInit);
}

TheApp::~TheApp()
{
	shvInfo() << "Destroying SHV BROKER application object";
	//QF_SAFE_DELETE(m_tcpServer);
	//QF_SAFE_DELETE(m_sqlConnector);
}

QString TheApp::versionString() const
{
	return QCoreApplication::applicationVersion();
}

#ifdef Q_OS_UNIX
void TheApp::installUnixSignalHandlers()
{
	shvInfo() << "installing Unix signals handlers";
	{
		struct sigaction term;

		term.sa_handler = TheApp::sigTermHandler;
		sigemptyset(&term.sa_mask);
		term.sa_flags |= SA_RESTART;

		if(sigaction(SIGTERM, &term, 0) > 0)
			qFatal("Couldn't register SIG_TERM handler");
	}
	if(::socketpair(AF_UNIX, SOCK_STREAM, 0, m_sigTermFd))
		qFatal("Couldn't create SIG_TERM socketpair");
	m_snTerm = new QSocketNotifier(m_sigTermFd[1], QSocketNotifier::Read, this);
	connect(m_snTerm, &QSocketNotifier::activated, this, &TheApp::handleSigTerm);
	shvInfo() << "SIG_TERM handler installed OK";
}

void TheApp::sigTermHandler(int)
{
	shvInfo() << "SIG TERM";
	char a = 1;
	::write(m_sigTermFd[0], &a, sizeof(a));
}

void TheApp::handleSigTerm()
{
	m_snTerm->setEnabled(false);
	char tmp;
	::read(m_sigTermFd[1], &tmp, sizeof(tmp));

	shvInfo() << "SIG TERM catched, stopping agent.";
	QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);

	m_snTerm->setEnabled(true);
}
#endif

void TheApp::lazyInit()
{
	checkConnected();
}

void TheApp::checkConnected()
{
	if(m_clientConnection) {
		if(!m_clientConnection->isConnected()) {
			m_clientConnection->abort();
			SHV_SAFE_DELETE(m_clientConnection);
			//m_clientConnection->deleteLater();
			m_clientConnection = nullptr;
		}
	}
	if(!m_clientConnection) {
		shvInfo().nospace() << "creating client connection to: " << m_cliOptions->userName() << "@" << m_cliOptions->serverHost() << ":" << m_cliOptions->serverPort();
		m_clientConnection = new rpc::ClientConnection(this);
		m_clientConnection->setUser(m_cliOptions->userName());
		m_clientConnection->connectToHost(m_cliOptions->serverHost(), m_cliOptions->serverPort());
	}
}

