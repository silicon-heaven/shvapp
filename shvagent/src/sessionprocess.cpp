#include "sessionprocess.h"

#include <shv/coreqt/log.h>

SessionProcess::SessionProcess(QObject *parent)
	: QProcess(parent)
{
	setCurrentReadChannel(QProcess::StandardError);

	connect(this, QOverload<int>::of(&QProcess::finished), this, &SessionProcess::onFinished);
	connect(this, &QProcess::readyReadStandardError, this, &SessionProcess::onReadyReadStandardError);
}

void SessionProcess::onFinished(int exit_code)
{
	shvInfo() << "Process" << program() << "finished with exit code:" << exit_code;
}

void SessionProcess::onReadyReadStandardError()
{
	QByteArray ba = readAll();
	shvWarning() << "Process stderr:" << std::string(ba.constData(), ba.size());
}
