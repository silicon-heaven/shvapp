#pragma once

#include <QProcess>

class SessionProcess : public QProcess
{
	Q_OBJECT

	using Super = QProcess;
public:
	SessionProcess(QObject *parent);
private:
	void onFinished(int exit_code, ExitStatus);
	//void onReadyReadStandardOutput();
	//void onReadyReadStandardError();

#if QT_VERSION_MAJOR < 6
	void setupChildProcess() override;
#endif
};
