#ifndef PROCESS_H
#define PROCESS_H

#include <QProcess>

class Process : public QProcess
{
	Q_OBJECT

	using Super = QProcess;
public:
	Process(QObject *parent = nullptr);
private:
#if QT_VERSION_MAJOR < 6
	void setupChildProcess() override;
#endif
};

#endif // PROCESS_H
