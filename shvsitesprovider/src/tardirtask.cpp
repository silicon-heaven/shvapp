#include "tardirtask.h"

TarDirTask::TarDirTask(const QString &dir_path, QObject *parent)
	: Super(parent)
	, m_tarProcess(new QProcess(this))
{
	m_tarProcess->setProgram("tar");
	m_tarProcess->setWorkingDirectory(dir_path);
	m_tarProcess->setArguments(QStringList{ "cfz", "-", "." });
	connect(m_tarProcess, &QProcess::readyReadStandardOutput, this, &TarDirTask::onReadyReadStandardOutput);
	connect(m_tarProcess, &QProcess::readyReadStandardError, this, &TarDirTask::onReadyReadStandardError);
	connect(m_tarProcess, &QProcess::errorOccurred, this, &TarDirTask::onError);
	connect(m_tarProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, &TarDirTask::onFinished);
}

shv::chainpack::RpcValue TarDirTask::result() const
{
	return m_result.toStdString();
}

std::string TarDirTask::error() const
{
	return m_errors.join('\n').toStdString();
}

void TarDirTask::onReadyReadStandardOutput()
{
	m_result += m_tarProcess->readAllStandardOutput();
}

void TarDirTask::onReadyReadStandardError()
{
	if (m_errors.count() == 0) {
		m_errors << QString();
	}
	m_errors[0] += QString::fromUtf8(m_tarProcess->readAllStandardError());
}

void TarDirTask::onFinished(int exit_code, QProcess::ExitStatus exit_status)
{
	if (exit_code == 0 && exit_status == QProcess::ExitStatus::NormalExit) {
		Q_EMIT finished(true);
	}
	else {
		if (m_errors.count()) {
			m_errors = m_errors[0].split('\n');
		}
		if (exit_code != 0) {
			m_errors << "Tar exit code " + QString::number(exit_code);
		}
		if (exit_status != QProcess::ExitStatus::CrashExit) {
			m_errors << "Tar crashed";
		}
		Q_EMIT finished(false);
	}
}

void TarDirTask::onError(QProcess::ProcessError error)
{
	Q_UNUSED(error);
	m_errors << m_tarProcess->errorString();
	if (m_tarProcess->state() == QProcess::ProcessState::Running) {
		m_tarProcess->terminate();
	}
	else {
		Q_EMIT finished(false);
	}
}
