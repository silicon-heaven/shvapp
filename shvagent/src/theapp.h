#pragma once

#include "appclioptions.h"

#include <QCoreApplication>
#include <QDateTime>

class QSocketNotifier;
class QTimer;

namespace rpc { class ClientConnection; }

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

	static TheApp* instance() {return qobject_cast<TheApp*>(Super::instance());}
private:
	void lazyInit();
	void checkConnected();
private:
	AppCliOptions *m_cliOptions;
	QTimer *m_checkConnectedTimer;
	rpc::ClientConnection *m_clientConnection = nullptr;
#ifdef Q_OS_UNIX
private:
	// Unix signal handlers.
	// You can't call Qt functions from Unix signal handlers,
	// but you can write to socket
	void installUnixSignalHandlers();
	static void sigTermHandler(int);
	Q_SLOT void handleSigTerm();
private:
	static int m_sigTermFd[2];
	QSocketNotifier *m_snTerm = nullptr;
#endif
};

