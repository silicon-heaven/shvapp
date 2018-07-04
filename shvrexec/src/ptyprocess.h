#pragma once

#include <QProcess>

#ifndef Q_OS_UNIX
#error Only POSIX platform supported
#endif

class QSocketNotifier;

class PtyProcess : public QProcess
{
	Q_OBJECT

	using Super = QProcess;
public:
	PtyProcess(QObject *parent = nullptr);
	~PtyProcess() Q_DECL_OVERRIDE;

	void ptyStart(const std::string &cmd, int pty_cols, int pty_rows);

	//void setTerminalWindowSize(int w, int h);

	qint64 writePtyMaster(const char *data, int len);

	Q_SIGNAL void readyReadMasterPty();
	std::vector<char> readAllMasterPty();
private:
	void openPty();
	void setupChildProcess() override;
	//int slavePtyFd();
private:
	int m_masterPtyFd = -1;
	int m_slavePtyFd = -1;
	std::string m_slavePtyName;
	//int m_sendSlavePtyFd[2];
	int m_ptyCols = 0;
	int m_ptyRows = 0;

	QSocketNotifier *m_masterPtyNotifier = nullptr;
};

