#include "untardirtask.h"

#include <QDir>

UntarDirTask::UntarDirTask(const QString &dir_path, const QByteArray &tar_content, QObject *parent)
	: Super(dir_path, parent)
	, m_tarContent(tar_content)
{
	if (!QDir(dir_path).exists()) {
		QDir::current().mkpath(dir_path);
	}
	m_tarProcess->setArguments(QStringList{ "xvfz", "-" });
	connect(m_tarProcess, &QProcess::started, this, &UntarDirTask::onStarted);
}

void UntarDirTask::onStarted()
{
	m_tarProcess->write(m_tarContent);
	m_tarProcess->closeWriteChannel();
}
