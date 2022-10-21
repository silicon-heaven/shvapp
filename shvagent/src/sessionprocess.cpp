#include "sessionprocess.h"

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
	connect(this, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &SessionProcess::onFinished);
	//connect(this, &QProcess::readyReadStandardOutput, this, &SessionProcess::onReadyReadStandardOutput);
	//connect(this, &QProcess::readyReadStandardError, this, &SessionProcess::onReadyReadStandardError);
}

void SessionProcess::onFinished(int exit_code, QProcess::ExitStatus)
{
	shvInfo() << "Process" << program() << arguments() << "finished with exit code:" << exit_code;
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
//#ifdef Q_OS_UNIX
//	// as a std user ends with
//	//Cannot make shvagent process the group leader, error set process group ID: 1 Operation not permitted
//	if(0 != ::setpgid(0, ::getppid()))
//		shvError() << "Error set process group ID:" << errno << ::strerror(errno);
//#endif
	Super::setupChildProcess();
}
