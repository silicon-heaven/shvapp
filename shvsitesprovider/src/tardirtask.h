#ifndef TARDIRTASK_H
#define TARDIRTASK_H

#include <QObject>
#include <QProcess>

#include <shv/chainpack/rpcvalue.h>

class TarDirTask : public QObject
{
	Q_OBJECT
	using Super = QObject;

public:
	TarDirTask(const QString &dir_path, QObject *parent);

	void start() { m_tarProcess->start(); }
	Q_SIGNAL void finished(bool success);

	shv::chainpack::RpcValue result() const;
	std::string error() const;

protected:
	void onReadyReadStandardOutput();
	void onReadyReadStandardError();
	void onFinished(int exit_code, QProcess::ExitStatus exit_status);
	void onError(QProcess::ProcessError error);

	QProcess *m_tarProcess;
	QByteArray m_result;
	QStringList m_errors;
};

#endif // TARDIRTASK_H
