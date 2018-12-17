#include "sessionprocess.h"
#include "shvagentapp.h"

#include <shv/coreqt/log.h>

#include <QCoreApplication>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif

SessionProcess::SessionProcess(QObject *parent)
	: QProcess(parent)
{
	//connect(ShvAgentApp::instance(), &ShvAgentApp::aboutToTerminate, this, &SessionProcess::terminate);
	connect(this, QOverload<int>::of(&QProcess::finished), this, &SessionProcess::onFinished);
	//connect(this, &QProcess::readyReadStandardOutput, this, &SessionProcess::onReadyReadStandardOutput);
	//connect(this, &QProcess::readyReadStandardError, this, &SessionProcess::onReadyReadStandardError);
}

void SessionProcess::onFinished(int exit_code)
{
	shvInfo() << "Process" << program() << "finished with exit code:" << exit_code;
	deleteLater();
}
/*
void SessionProcess::onReadyReadStandardOutput()
{
	//setCurrentReadChannel(QProcess::StandardOutput);
	QByteArray ba = readAllStandardOutput();
	if(!ba.isEmpty())
		shvInfo() << "Process stdout:" << std::string(ba.constData(), ba.size());
}
*/
/*
void SessionProcess::onReadyReadStandardError()
{
	//setCurrentReadChannel(QProcess::StandardError);
	QByteArray ba = readAllStandardError().trimmed();
	if(!ba.isEmpty())
		shvWarning() << "Process stderr:" << std::string(ba.constData(), ba.size());
}
*/
void SessionProcess::setupChildProcess()
{
#ifdef Q_OS_UNIX
	if(0 != ::setpgid(0, ::getppid()))
		shvError() << "Error set process group ID:" << errno << ::strerror(errno);
#endif
	Super::setupChildProcess();
}
