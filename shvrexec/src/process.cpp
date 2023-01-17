#include "process.h"

#ifdef Q_OS_LINUX
#include <signal.h>
#include <sys/prctl.h>
#endif

Process::Process(QObject *parent)
	: Super(parent)
{
#ifdef Q_OS_LINUX
	setChildProcessModifier([] {
		::prctl(PR_SET_PDEATHSIG, SIGHUP);
	});
#endif

}
