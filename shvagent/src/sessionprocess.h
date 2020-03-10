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

	void setupChildProcess() override;
};
