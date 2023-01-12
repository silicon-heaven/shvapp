#ifndef PROCESS_H
#define PROCESS_H

#include <QProcess>

class Process : public QProcess
{
	Q_OBJECT

	using Super = QProcess;
public:
	Process(QObject *parent = nullptr);
};

#endif // PROCESS_H
