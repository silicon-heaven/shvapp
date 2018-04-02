#pragma once

#include <QProcess>

class SessionProcess : public QProcess
{
	Q_OBJECT
public:
	SessionProcess(QObject *parent);
private:
	void onFinished(int exit_code);
	void onReadyReadStandardError();
};
