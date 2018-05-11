#include "childprocess.h"

#include <shv/coreqt/log.h>

#ifdef Q_OS_UNIX
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif

ChildProcess::ChildProcess(QObject *parent)
	: Super(parent)
{

}

void ChildProcess::setupChildProcess()
{
#ifdef Q_OS_UNIX
	if(0 != ::setpgid(0, ::getppid()))
		shvError() << "Error set process group ID:" << errno << ::strerror(errno);
#endif
	Super::setupChildProcess();
}
