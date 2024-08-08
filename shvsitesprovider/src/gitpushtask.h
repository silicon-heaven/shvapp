// #ifndef GITPUSHTASK_H
// #define GITPUSHTASK_H

// #include "nodes.h"

// #include <QTimer>

// class GitPushTask : public QObject
// {
// 	Q_OBJECT

// public:
// 	enum Step { Initial, Pull, Add, Commit, Push, Finished };

// 	GitPushTask(const QString &user, AppRootNode *parent);

// 	const QString &error() const { return m_error; }

// 	void start();
// 	Q_SIGNAL void finished(bool success);

// private:
// 	void timeout();
// 	void execStep();
// 	void execGitCommand(const QStringList &arguments);
// 	void onGitCommandFinished();

// 	Step m_step;
// 	QString m_user;
// 	QString m_error;
// 	QTimer m_timeoutTimer;
// };

// #endif // GITPUSHTASK_H
