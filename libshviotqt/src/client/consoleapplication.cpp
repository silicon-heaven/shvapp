#include "consoleapplication.h"
#include "clientconnection.h"
#include "appclioptions.h"

#include <shv/coreqt/log.h>
#include <shv/coreqt/utils/versioninfo.h>

#include <QSocketNotifier>
#include <QTimer>

#ifdef Q_OS_UNIX
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

//#define logOpcuaReceive qfCInfo("OpcuaReceive")

namespace shv {
namespace iotqt {
namespace client {

#ifdef Q_OS_UNIX
int ConsoleApplication::m_sigTermFd[2];
#endif

//static constexpr int SQL_RECONNECT_INTERVAL = 3000;

ConsoleApplication::ConsoleApplication(int &argc, char **argv, AppCliOptions *cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	shvInfo() << "creating SHV client application object ver." << versionString();
#ifdef Q_OS_UNIX
	//syslog (LOG_INFO, "Server started");
	installUnixSignalHandlers();
#endif

	m_clientConnection = new Connection(this);
	//m_clientConnection->setProfile(profile());
	//m_clientConnection->setDeviceId(deviceId());
	m_clientConnection->setUser(m_cliOptions->userName().toStdString());

	m_checkConnectedTimer = new QTimer(this);
	m_checkConnectedTimer->start(1000 * 10);
	connect(m_checkConnectedTimer, &QTimer::timeout, this, &ConsoleApplication::checkConnected);

	QTimer::singleShot(0, this, &ConsoleApplication::lazyInit);
}

ConsoleApplication::~ConsoleApplication()
{
	shvInfo() << "Destroying SHV CLIENT application object";
	//QF_SAFE_DELETE(m_tcpServer);
	//QF_SAFE_DELETE(m_sqlConnector);
}

coreqt::utils::VersionInfo ConsoleApplication::versionInfo() const
{
	return shv::coreqt::utils::VersionInfo(versionString());
}

QString ConsoleApplication::versionString() const
{
	return QCoreApplication::applicationVersion();
}

#ifdef Q_OS_UNIX
void ConsoleApplication::installUnixSignalHandlers()
{
	shvInfo() << "installing Unix signals handlers";
	{
		struct sigaction term;

		term.sa_handler = ConsoleApplication::sigTermHandler;
		sigemptyset(&term.sa_mask);
		term.sa_flags |= SA_RESTART;

		if(sigaction(SIGTERM, &term, 0) > 0)
			qFatal("Couldn't register SIG_TERM handler");
	}
	if(::socketpair(AF_UNIX, SOCK_STREAM, 0, m_sigTermFd))
		qFatal("Couldn't create SIG_TERM socketpair");
	m_snTerm = new QSocketNotifier(m_sigTermFd[1], QSocketNotifier::Read, this);
	connect(m_snTerm, &QSocketNotifier::activated, this, &ConsoleApplication::handleSigTerm);
	shvInfo() << "SIG_TERM handler installed OK";
}

void ConsoleApplication::sigTermHandler(int)
{
	shvInfo() << "SIG TERM";
	char a = 1;
	::write(m_sigTermFd[0], &a, sizeof(a));
}

void ConsoleApplication::handleSigTerm()
{
	m_snTerm->setEnabled(false);
	char tmp;
	::read(m_sigTermFd[1], &tmp, sizeof(tmp));

	shvInfo() << "SIG TERM catched, stopping agent.";
	QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);

	m_snTerm->setEnabled(true);
}
#endif

void ConsoleApplication::lazyInit()
{
	checkConnected();
}

void ConsoleApplication::checkConnected()
{
	if(!m_clientConnection->isSocketConnected()) {
		m_clientConnection->abort();
		shvInfo().nospace() << "connecting to: " << m_cliOptions->userName() << "@" << m_cliOptions->serverHost() << ":" << m_cliOptions->serverPort();
		//m_clientConnection->setProtocolVersion(protocolVersion());
		m_clientConnection->connectToHost(m_cliOptions->serverHost().toStdString(), m_cliOptions->serverPort());
	}
}

} // namespace client
} // namespace iot
} // namespace shv
