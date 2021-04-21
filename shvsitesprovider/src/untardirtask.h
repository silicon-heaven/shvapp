#ifndef UNTARDIRTASK_H
#define UNTARDIRTASK_H

#include "tardirtask.h"

class UntarDirTask : public TarDirTask
{
	Q_OBJECT
	using Super = TarDirTask;

public:
	UntarDirTask(const QString &dir_path, const QByteArray &tar_content, QObject *parent);

protected:
	void onStarted();

	QByteArray m_tarContent;
};

#endif // UNTARDIRTASK_H
