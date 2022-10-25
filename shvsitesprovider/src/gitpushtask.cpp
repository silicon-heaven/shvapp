#include "gitpushtask.h"

#include "appclioptions.h"
#include "sitesproviderapp.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/deviceconnection.h>
#include <shv/iotqt/rpc/rpc.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>

#include <QProcess>

namespace cp = shv::chainpack;

GitPushTask::GitPushTask(const QString &user, AppRootNode *parent)
	: QObject(parent)
	, m_step(Initial)
	, m_user(user)
	, m_timeoutTimer(this)
{
	connect(&m_timeoutTimer, &QTimer::timeout, this, &GitPushTask::timeout);
	connect(this, &GitPushTask::finished, this, &GitPushTask::deleteLater);
	m_timeoutTimer.setSingleShot(true);
	m_timeoutTimer.start(5 * 60 * 1000);
}

void GitPushTask::start()
{
	m_step = Pull;
	execStep();
}

void GitPushTask::execGitCommand(const QStringList &arguments)
{
	QProcess *command = new QProcess(this);
	connect(command, &QProcess::errorOccurred, [this, command](QProcess::ProcessError error) {
		if (error == QProcess::FailedToStart) {
			m_error = command->errorString();
			Q_EMIT finished(false);
		}
	});
	connect(command, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [this, command](int exit_code, QProcess::ExitStatus exit_status){
		if (exit_status == QProcess::ExitStatus::NormalExit && exit_code == 0) {
			onGitCommandFinished();
		}
		else {
			shvError() << "error on git command" << command->errorString();
			if (exit_code != 0) {
				m_error = command->readAllStandardError();
			}
			else {
				m_error = command->errorString();
			}
			Q_EMIT finished(false);
		}
	});
	QString sites_dir = QString::fromStdString(SitesProviderApp::instance()->cliOptions()->localSitesDir());
	command->start("git", QStringList{ "-C", sites_dir } + arguments);
}

void GitPushTask::onGitCommandFinished()
{
	m_step = Step(static_cast<int>(m_step) + 1);
	if (m_step == Finished) {
		Q_EMIT finished(true);
	}
	else {
		execStep();
	}
}

void GitPushTask::execStep()
{
	QStringList arguments;
	switch (m_step) {
	case Pull:
		arguments = QStringList{ "pull" };
		break;
	case Add:
		arguments = QStringList{ "add", "./" };
		break;
	case Commit:
		arguments = QStringList{ "commit", "-a", "-m", "commit from sitesprovider by " + m_user };
		break;
	case Push:
		arguments = QStringList{ "push" };
		break;
	default:
		break;
	}
	Q_ASSERT(arguments.count());
	execGitCommand(arguments);
}

void GitPushTask::timeout()
{
	m_error = "Timeout";
	Q_EMIT finished(false);
}
