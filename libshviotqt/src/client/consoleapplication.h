#pragma once

#include <shv/core/utils.h>
#include <shv/chainpack/rpcvalue.h>

#include <QCoreApplication>

#include "../shviotqtglobal.h"

namespace shv { namespace coreqt { namespace utils { class VersionInfo; }}}

class QTimer;
class QSocketNotifier;

namespace shv {
namespace iotqt {
namespace client {

class AppCliOptions;
class Connection;

class SHVIOTQT_DECL_EXPORT ConsoleApplication : public QCoreApplication
{
	Q_OBJECT
	using Super = QCoreApplication;

	//SHV_FIELD_IMPL(int, p, P, rotocolVersion)
	//SHV_FIELD_IMPL(std::string, p, P, rofile)
	//SHV_FIELD_IMPL(shv::chainpack::RpcValue, d, D, eviceId)
public:
	ConsoleApplication(int &argc, char **argv, AppCliOptions* cli_opts);
	~ConsoleApplication() Q_DECL_OVERRIDE;

	coreqt::utils::VersionInfo versionInfo() const;
	QString versionString() const;

	AppCliOptions* cliOptions() {return m_cliOptions;}
	Connection *clientConnection() {return m_clientConnection;}

	static ConsoleApplication* instance() {return qobject_cast<ConsoleApplication*>(Super::instance());}
protected:
	void lazyInit();
	void checkConnected();
protected:
	AppCliOptions *m_cliOptions;
	QTimer *m_checkConnectedTimer;
	Connection *m_clientConnection = nullptr;
#ifdef Q_OS_UNIX
protected:
	// Unix signal handlers.
	// You can't call Qt functions from Unix signal handlers,
	// but you can write to socket
	void installUnixSignalHandlers();
	static void sigTermHandler(int);
	Q_SLOT void handleSigTerm();
protected:
	static int m_sigTermFd[2];
	QSocketNotifier *m_snTerm = nullptr;
#endif
};

} // namespace client
} // namespace iot
} // namespace shv
