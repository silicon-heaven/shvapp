#include "sessionprocess.h"

#include <shv/coreqt/log.h>

SessionProcess::SessionProcess(QObject *parent)
	: QProcess(parent)
{
	connect(this, QOverload<int>::of(&QProcess::finished), this, &SessionProcess::onFinished);
	connect(this, &QProcess::readyReadStandardError, this, &SessionProcess::onReadyReadStandardError);
}

void SessionProcess::onFinished(int exit_code)
{
	shvInfo() << "Process" << program() << "finished with exit code:" << exit_code;
}

void SessionProcess::onReadyReadStandardOutput()
{
	setCurrentReadChannel(QProcess::StandardOutput);
	QByteArray ba = readAll();
	if(!ba.isEmpty())
		shvInfo() << "Process stdout:" << std::string(ba.constData(), ba.size());
}

void SessionProcess::onReadyReadStandardError()
{
	setCurrentReadChannel(QProcess::StandardError);
	QByteArray ba = readAll();
	if(!ba.isEmpty())
		shvWarning() << "Process stderr:" << std::string(ba.constData(), ba.size());
}
