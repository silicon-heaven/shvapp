#ifndef CHILDPROCESS_H
#define CHILDPROCESS_H

#include <QProcess>

class ChildProcess : public QProcess
{
	Q_OBJECT

	using Super = QProcess;
public:
	ChildProcess(QObject *parent = nullptr);
private:
	void setupChildProcess() override;
};

#endif // CHILDPROCESS_H
